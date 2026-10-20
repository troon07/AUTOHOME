"""Resume the pending GPIO-inhibited campaign when both boards return.

Optionally deploys the reviewed A and B images; never changes phone/network settings
or enables output pins. Measurements require matching last-flash provenance.
The bridge child belongs to this script and is stopped on completion/failure.
"""
import argparse
import hashlib
import json
import subprocess
import sys
import time
import urllib.request
from pathlib import Path
from serial.tools import list_ports

ROOT=Path(__file__).resolve().parents[1]

def verified_flash_artifact(artifact, manifests):
    """Reject missing, changed or unsuccessfully last-flashed applications."""
    paths=sorted(manifests)
    if not paths or not artifact.is_file(): return False
    latest=json.loads(paths[-1].read_text())
    digest=hashlib.sha256(artifact.read_bytes()).hexdigest()
    return latest.get('exit_code')==0 and any(item.get('address')=='0x10000' and item.get('sha256')==digest for item in latest.get('artifacts',[]))

def main(deploy_staged=False):
    ports={p.device for p in list_ports.comports()}
    if not {'COM5','COM7'} <= ports:
        print('Both COM5 and COM7 are required. No board access attempted.')
        return 2
    if deploy_staged:
        for role in ['A','B-speech']:
            result=subprocess.run([sys.executable,'scripts/flash_native.py','--role',role],cwd=ROOT)
            if result.returncode: return result.returncode
        time.sleep(2)
    raw=ROOT/'benchmarks/raw/hardware'
    for project,env in [('native','native_voice'),('speech','speech_intelligence')]:
        if not verified_flash_artifact(ROOT/project/'.pio/build'/env/'firmware.bin',raw.glob(f'flash_{env}_*.json')):
            print(f'{env}: build differs from last successful flash. Deploy the reviewed image before attributing measurements to it. For the staged B UI fix, use --deploy-staged.')
            return 2
    commands=[['scripts/research_hil.py','--simulation-only','--speech','--trials','1000','--soak-seconds','60'],
              ['scripts/recovery_hil.py']]
    for command in commands:
        result=subprocess.run([sys.executable,*command],cwd=ROOT)
        if result.returncode: return result.returncode
    # Use a separate port from the offline interface preview.
    try:
        urllib.request.urlopen('http://127.0.0.1:8766/simulation',timeout=1)
    except OSError: pass
    else:
        print('Port 8766 is already in use; stop that service before resuming.')
        return 2
    log=ROOT/'benchmarks/raw/resume_bridge.log'
    with log.open('w') as output:
        bridge=subprocess.Popen([sys.executable,'-u','scripts/browser_bridge.py','--serial','COM7','--port','8766'],cwd=ROOT,stdout=output,stderr=subprocess.STDOUT)
        try:
            for _ in range(30):
                if bridge.poll() is not None: raise RuntimeError('Bridge failed; inspect resume_bridge.log')
                try:
                    with urllib.request.urlopen('http://127.0.0.1:8766/simulation',timeout=.5) as response:
                        if response.status==200: break
                except OSError: time.sleep(.2)
            else: raise RuntimeError('Bridge startup timed out')
            return subprocess.run([sys.executable,'scripts/audio_bench.py','--url','http://127.0.0.1:8766'],cwd=ROOT).returncode
        finally:
            bridge.terminate()
            try: bridge.wait(timeout=5)
            except subprocess.TimeoutExpired: bridge.kill(); bridge.wait()

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--deploy-staged',action='store_true',help='Deploy reviewed A and B-speech images/model before validation; preserves NVS and verifies MACs')
    raise SystemExit(main(parser.parse_args().deploy_staged))
