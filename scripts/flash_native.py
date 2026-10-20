"""Flash reviewed local artifacts with esptool 5.4, preserving NVS.

The PlatformIO-bundled esptool 4.5 was unreliable on this USB setup. No NVS erase,
eFuse programming, secure-boot changes or credential output is performed.
"""
import argparse
import hashlib
import json
import re
import subprocess
import sys
from datetime import datetime,timezone
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]


def main(args):
    speech=args.role=='B-speech'; a=args.role=='A'
    port=args.port or ('COM5' if a else 'COM7')
    expected='e0:72:a1:f6:a9:68' if a else 'e0:72:a1:f5:bc:b0'
    project=ROOT/('speech' if speech else 'native')
    env='speech_intelligence' if speech else 'native_voice' if a else 'native_intelligence'
    build=project/'.pio/build'/env
    sdk=(project/('sdkconfig.'+env)).read_text()
    required=['CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y','CONFIG_ESP32S3_SPIRAM_SUPPORT=y','CONFIG_SPIRAM_MODE_OCT=y']
    if any(line not in sdk.splitlines() for line in required): raise RuntimeError('Expected N16R8 SDK configuration is missing')
    files=[(0,build/'bootloader.bin'),(0x8000,build/'partitions.bin'),(0xe000,build/'ota_data_initial.bin'),(0x10000,build/'firmware.bin')]
    if args.app_only:
        files=[(0x10000,build/'firmware.bin')]
    if speech and not args.app_only:
        subprocess.run([sys.executable,str(project/'components/esp-sr/model/movemodel.py'),'-d1',str(project/('sdkconfig.'+env)),'-d2',str(project/'components/esp-sr'),'-d3',str(build)],check=True)
        model=build/'srmodels/srmodels.bin'
        if model.stat().st_size>0x600000: raise RuntimeError('Speech models exceed reserved partition')
        files.append((0x610000,model))
    if not all(path.is_file() for _,path in files): raise RuntimeError('Build artifacts missing; compile first')
    command=[sys.executable,'-m','esptool','--chip','esp32s3','--port',port,'--baud','460800']
    identity=subprocess.run(command+['--after','no-reset','read-mac'],capture_output=True,text=True,timeout=30)
    if identity.returncode or expected not in identity.stdout.lower(): raise RuntimeError('Board MAC identity did not match its assigned role')
    stamp=datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ')
    log=ROOT/f'benchmarks/raw/hardware/flash_{env}_{stamp}.log'; log.parent.mkdir(parents=True,exist_ok=True)
    arguments=['write-flash','--flash-mode','dio','--flash-freq','80m','--flash-size','16MB']
    for address,path in files: arguments += [hex(address),str(path)]
    with log.open('w') as output:
        output.write(identity.stdout); output.flush()
        result=subprocess.run(command+arguments,stdout=output,stderr=subprocess.STDOUT,timeout=180)
    manifest=dict(role=args.role,port=port,mac=expected,tool='esptool 5.4.0',exit_code=result.returncode,
                  artifacts=[dict(address=hex(address),path=str(path.relative_to(ROOT)),sha256=hashlib.sha256(path.read_bytes()).hexdigest()) for address,path in files])
    log.with_suffix('.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print(f'Flash {"PASS" if result.returncode==0 else "FAIL"}: {args.role}; log {log}',flush=True)
    return result.returncode


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__); parser.add_argument('--role',choices=['A','B','B-speech'],required=True); parser.add_argument('--port')
    parser.add_argument('--app-only',action='store_true',help='Development app0 rewrite; requires the existing verified partition/boot/model images')
    raise SystemExit(main(parser.parse_args()))
