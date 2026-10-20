"""Read-only/API rejection and shadow-monitor tests on Node B, without A."""
import argparse
import csv
import hashlib
import json
import sys
import urllib.error
import urllib.request
from datetime import datetime,timezone
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]; sys.path.insert(0,str(ROOT))
from scripts.resume_validation import verified_flash_artifact

NAMES=['initial_unknown','missing_feedback_timeout','wrong_boot','wrong_command','future_sample','stale_sample',
       'pre_command_sample','duplicate_no_renewal','missing_modality','shared_fault_domain','repeated_command_id',
       'fresh_agreement','lease_expiry_unknown','contradiction_latches','fault_cannot_clear_on','confirmed_off_manual_clear',
       'deadline_cannot_be_repaired_late','uint32_clock_wrap','sample_gap_breaks_stability','backward_sample_time',
       'duplicate_generation_time','1024_value_availability_domain_clock_vectors','new_id_cannot_extend_pending_deadline']

def main(args):
    image=ROOT/'speech/.pio/build/speech_intelligence/firmware.bin'
    if not verified_flash_artifact(image,(ROOT/'benchmarks/raw/hardware').glob('flash_speech_intelligence_*.json')):
        raise RuntimeError('Deploy the reviewed B image before attributing tests to it')
    token=json.loads((ROOT/'local_credentials.json').read_text())['api_token']; cases=[]; records={}; errors=[]
    def request(path,body=None,authenticated=True):
        headers={'Content-Type':'application/octet-stream'}
        if authenticated: headers['X-ClawGrid-Token']=token
        req=urllib.request.Request(args.url+path,data=body,headers=headers)
        try:
            with urllib.request.urlopen(req,timeout=15) as response: return response.status,json.load(response)
        except urllib.error.HTTPError as error: return error.code,error.read().decode(errors='replace')
    def case(name,ok):
        cases.append(dict(test=name,execution_source='HARDWARE_MEASURED',input_source='SIMULATED',status='PASS' if ok else 'FAIL'))
        print(name,cases[-1]['status'],flush=True)
    try:
        for name in ['speech','memory','rtos','state','evidence']:
            status,body=request('/api/'+name); records[name]=body; case('api_'+name,status==200)
        evidence=records['evidence']
        if evidence.get('cases')!=len(NAMES): raise RuntimeError('Host/firmware case registry mismatch')
        for i,name in enumerate(NAMES): case(name,bool(evidence['passed_mask']&(1<<i)))
        status,_=request('/api/evidence',authenticated=False); case('monitor_requires_token',status==401)
        before=records['speech']; status,_=request('/api/speech/audio',b'bad')
        _,after=request('/api/speech'); case('short_pcm_rejected_without_job',status==400 and after.get('job')==before.get('job'))
        case('speech_model_ready',after.get('supported') and after.get('phase') in (1,4))
        case('memory_config_ok',records['memory'].get('memory_config_ok'))
    except Exception as error: errors.append(repr(error)); case('runner_exception',False)
    stamp=datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ'); path=ROOT/f'benchmarks/raw/hardware/single_b_{stamp}.json'
    payload=dict(execution_source='HARDWARE_MEASURED',input_source='SIMULATED',setup='COM7 Node B only; no A, sensors, relays or physical feedback',
                 firmware_sha256=hashlib.sha256(image.read_bytes()).hexdigest(),cases=cases,records=records,errors=errors)
    path.write_text(json.dumps(payload,indent=2)+'\n')
    with (ROOT/'benchmarks/reports/SINGLE_BOARD_B_RESULTS.csv').open('w',newline='') as f:
        writer=csv.DictWriter(f,fieldnames=list(cases[0])); writer.writeheader(); writer.writerows(cases)
    print('Evidence:',path); return int(bool(errors) or any(c['status']=='FAIL' for c in cases))

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__); parser.add_argument('--url',default='http://127.0.0.1:8766')
    raise SystemExit(main(parser.parse_args()))
