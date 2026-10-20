"""Evaluate synthetic PCM on ESP32 MultiNet; never run speech inference here.

The localhost bridge forwards bytes. Accuracy here is a synthetic smoke test,
not a human-speaker evaluation or evidence that one model is best.
"""
import argparse
import csv
import hashlib
import json
import random
import struct
import sys
import time
import urllib.request
import urllib.error
import wave
import zlib
from datetime import datetime,timezone
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT))


def run(args):
    from scripts.resume_validation import verified_flash_artifact
    targets=[('speech','speech_intelligence')] if args.speech_only else [('native','native_voice'),('speech','speech_intelligence')]
    for project,env in targets:
        if not verified_flash_artifact(ROOT/project/'.pio/build'/env/'firmware.bin',(ROOT/'benchmarks/raw/hardware').glob(f'flash_{env}_*.json')):
            raise RuntimeError(f'{env}: build differs from last successful flash; deploy before recording metrics')
    token=json.loads((ROOT/'local_credentials.json').read_text())['api_token']
    def request(path,body=None):
        req=urllib.request.Request(args.url+path,data=body,headers={'X-ClawGrid-Token':token,'Content-Type':'application/octet-stream'})
        try:
            with urllib.request.urlopen(req,timeout=20) as response: return json.load(response)
        except urllib.error.HTTPError as error:
            raise RuntimeError(f'{path}: HTTP {error.code}: {error.read(2048).decode(errors="replace")}') from error
    dataset=Path(args.dataset).resolve(); manifest=list(csv.DictReader((dataset/'manifest.csv').open(encoding='utf-8-sig')))
    clips=[]
    for row in manifest:
        path=(dataset/row['file']).resolve()
        if not path.is_relative_to(dataset): raise ValueError('Clip outside dataset')
        with wave.open(str(path),'rb') as audio:
            if (audio.getframerate(),audio.getnchannels(),audio.getsampwidth())!=(16000,1,2): raise ValueError('PCM format mismatch')
            pcm=audio.readframes(audio.getnframes())
        if not 3200<=len(pcm)<=128000: raise ValueError('Clip exceeds firmware limit')
        clips.append((row['file'],int(row['expected_id']),pcm))
    rng=random.Random(20261020)
    clips += [('silence',0,bytes(64000)),('white_noise',0,b''.join(struct.pack('<h',rng.randint(-1000,1000)) for _ in range(32000)))]
    if args.limit is not None:
        if args.limit<1: raise ValueError('--limit must be positive')
        clips=clips[:args.limit]
    results=[]; errors=[]
    try:
        engine=request('/api/speech')
        if not engine.get('supported') or engine.get('phase') not in (1,4): raise RuntimeError('ESP32 speech engine not ready')
        for name,expected,pcm in clips:
            start=time.perf_counter_ns(); queued=request('/api/speech/audio',pcm)
            # The inference task may already be active when this OFF request
            # arrives. Report the observed phase rather than assuming overlap.
            phase_before=request('/api/speech')
            command=None if args.speech_only else request('/api/control',b'1 0 0 0'); control=None
            deadline=time.monotonic()+15; detected=None
            while time.monotonic()<deadline:
                state=request('/api/speech')
                transaction={} if args.speech_only else request('/api/transaction')
                if command and transaction.get('id')==command.get('id') and transaction.get('phase')==2: control=transaction
                if state.get('job')==queued.get('job') and state.get('phase')==4:
                    detected=state
                    if args.speech_only or control: break
                time.sleep(.03)
            if not detected: raise RuntimeError('Speech job did not finish')
            if detected.get('pcm_crc32') != zlib.crc32(pcm):
                raise RuntimeError(f'{name}: board PCM checksum mismatch; recognition quality is not evaluable')
            result=dict(name=name,audio_source='SIMULATED',processing_source='HARDWARE_MEASURED',expected_id=expected,
                        recognized_id=detected['command_id'],correct=detected['command_id']==expected,
                        pcm_sha256=hashlib.sha256(pcm).hexdigest(),samples=len(pcm)//2,
                        browser_transport_source='HOST_MEASURED',host_job_ms=(time.perf_counter_ns()-start)/1e6,
                        board=detected,control=control,phase_before_control=phase_before.get('phase'),
                        memory=request('/api/memory'),rtos=request('/api/rtos'))
            results.append(result)
            print(name, 'PASS' if result['correct'] else 'MISMATCH',f'expected={expected} got={detected["command_id"]}',flush=True)
    except Exception as e: errors.append(repr(e)); print('Audio bench error:',type(e).__name__,flush=True)
    stamp=datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ')
    path=ROOT/f'benchmarks/raw/hardware/audio_{stamp}.json'
    artifacts={'B':'speech/.pio/build/speech_intelligence/firmware.bin','model':'speech/.pio/build/speech_intelligence/srmodels/srmodels.bin'}
    if not args.speech_only: artifacts['A']='native/.pio/build/native_voice/firmware.bin'
    hashes={name:hashlib.sha256((ROOT/file).read_bytes()).hexdigest() for name,file in artifacts.items()}
    data=dict(audio_source='SIMULATED',processing_source='HARDWARE_MEASURED',setup='Node B only; no actuator/control test' if args.speech_only else 'Dual-board speech and control',firmware_sha256=hashes,results=results,errors=errors,
              limitations='One synthetic voice; no independent human speakers, acoustic room/noise/distance validation, or model comparison. No accuracy generalization.')
    path.write_text(json.dumps(data,indent=2)+'\n')
    output=ROOT/('benchmarks/reports/AUDIO_B_ONLY_RESULTS.csv' if args.speech_only else 'benchmarks/reports/AUDIO_TEST_RESULTS.csv')
    with output.open('w',newline='') as f:
        writer=csv.writer(f); writer.writerow(['clip','audio_source','processing_source','expected','recognized','correct','inference_us','max_frame_us','stack_free_bytes','control_rtt_us','phase_before_control'])
        for r in results:
            s=r['board']; writer.writerow([r['name'],'SIMULATED','HARDWARE_MEASURED',r['expected_id'],r['recognized_id'],r['correct'],s['inference_us'],s['max_frame_us'],s['stack_free_bytes'],(r['control'] or {}).get('rtt_us'),r['phase_before_control']])
    print('Evidence:',path,flush=True)
    return 1 if errors or any(not r['correct'] for r in results) else 0


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__); p.add_argument('--url',default='http://127.0.0.1:8765'); p.add_argument('--dataset',default=str(ROOT/'benchmarks/data/synthetic_speech'))
    p.add_argument('--speech-only',action='store_true',help='Test B recognition only; no commands, Node A hash or control-concurrency claim')
    p.add_argument('--limit',type=int,help='Run only the first N inputs for diagnostics; not a full corpus result')
    raise SystemExit(run(p.parse_args()))
