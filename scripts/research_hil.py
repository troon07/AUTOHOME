"""Reproducible bare-board firmware tests. GPIO changes require --bare-boards.

This measures software state and radio acknowledgements, not physical loads,
speech, occupancy accuracy or energy. Every observation is retained.
"""
import argparse
import csv
import hashlib
import json
import sys
import time
from contextlib import ExitStack
from datetime import datetime,timezone
from pathlib import Path
import serial

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT))
from scripts.research_stats import summarize,wilson

class Bench:
    def __init__(self,ports,native=False,simulation_only=False,speech=False):
        self.stack=ExitStack(); self.connections={}; self.buffers={p:bytearray() for p in ports}
        self.records=[]; self.lines=[]; self.cases=[]; self.trials=[]
        self.native=native; self.simulation_only=simulation_only
        self.speech=speech
        journal_dir=ROOT/'benchmarks/raw/hardware'; journal_dir.mkdir(parents=True,exist_ok=True)
        journal_stamp=datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S%fZ')
        self.journal=self.stack.enter_context((journal_dir/f'live_{journal_stamp}.jsonl').open('w'))
        self.text_log=self.stack.enter_context((journal_dir/f'live_{journal_stamp}.log').open('w'))
        for port in ports:
            connection=serial.Serial(port=None,baudrate=115200,timeout=0,write_timeout=1)
            connection.dtr=False; connection.rts=False; connection.port=port; connection.open()
            self.stack.callback(connection.close); self.connections[port]=connection
        self.a,self.b=ports
    def poll(self,seconds=0.05):
        deadline=time.monotonic()+seconds
        while time.monotonic()<deadline:
            for port,connection in self.connections.items():
                self.buffers[port].extend(connection.read(min(connection.in_waiting,4096)))
                while b'\n' in self.buffers[port]:
                    raw,_,rest=self.buffers[port].partition(b'\n'); self.buffers[port]=bytearray(rest)
                    line=raw.decode(errors='replace').strip(); self.lines.append(f'{port} {line}')
                    self.text_log.write(f'{port} {line}\n'); self.text_log.flush()
                    try: record=json.loads(line)
                    except json.JSONDecodeError: continue
                    if isinstance(record,dict):
                        observation=dict(port=port,host_monotonic_ns=time.monotonic_ns(),device=record)
                        self.records.append(observation); self.journal.write(json.dumps(observation)+'\n'); self.journal.flush()
                if len(self.buffers[port])>16384: raise RuntimeError('Oversized serial line')
            time.sleep(.005)
    def wait(self,predicate,timeout=3,start=None):
        index=len(self.records) if start is None else start; deadline=time.monotonic()+timeout
        while time.monotonic()<deadline:
            self.poll(.01)
            for record in self.records[index:]:
                if predicate(record): return record['device']
            index=len(self.records)
        return None
    def request(self,port,command,kind,timeout=3):
        index=len(self.records); self.connections[port].write(command)
        return self.wait(lambda r:r['port']==port and r['device'].get('kind')==kind,timeout,index)
    def case(self,name,passed,details=''):
        self.cases.append(dict(test=name,source='HARDWARE_MEASURED',status='PASS' if passed else 'FAIL',details=details))
        self.journal.write(json.dumps(dict(kind='case',**self.cases[-1]))+'\n'); self.journal.flush()
        print(f'{name}: {"PASS" if passed else "FAIL"}',flush=True)
    def command(self,name,fields,expected=0,trial=False):
        index=len(self.records); self.connections[self.b].write(('C'+fields+'\n').encode())
        queued=self.wait(lambda r:r['port']==self.b and r['device'].get('kind')=='queued',3,index)
        result=None
        capture_recovery=False
        if queued and queued.get('accepted'):
            result=self.wait(lambda r:r['port']==self.b and r['device'].get('kind')=='transaction' and r['device'].get('id')==queued['id'],3,index)
            if result is None:
                # Recover delayed USB diagnostic delivery without reissuing an
                # actuator command. Device RTT remains the measurement source.
                capture_recovery=True; self.connections[self.b].write(b'S')
                result=self.wait(lambda r:r['port']==self.b and r['device'].get('kind')=='transaction' and r['device'].get('id')==queued['id'],3,index)
        passed=bool(result and result.get('phase')==2 and result.get('status')==expected)
        if trial:
            self.trials.append(dict(name=name,expected=expected,passed=passed,result=result,capture_recovery_requested=capture_recovery))
            self.journal.write(json.dumps(dict(kind='trial',**self.trials[-1]))+'\n'); self.journal.flush()
        else: self.case(name,passed,json.dumps(result))
        self.poll(.03)
        return result
    def state(self): return self.request(self.b,b'S','state')
    def http(self,method,path,body=b'',timeout=12):
        token=json.loads((ROOT/'local_credentials.json').read_text())['api_token']
        request_id=len(self.records)+1
        header=f'@{request_id} {"G" if method=="GET" else "P"} {path} {len(body)} {token}\n'.encode()
        index=len(self.records); self.connections[self.b].write(header)
        if body: self.connections[self.b].write(body)
        response=self.wait(lambda r:r['port']==self.b and r['device'].get('kind')=='bridge' and r['device'].get('id')==request_id,timeout,index)
        return response
    def save(self,stamp,errors):
        out=ROOT/'benchmarks/raw/hardware'; out.mkdir(parents=True,exist_ok=True)
        build_root=ROOT/'native/.pio/build' if self.native else ROOT/'.pio/build'
        environments=('native_voice','native_intelligence') if self.native else ('node_voice','node_intelligence')
        artifacts={env:build_root/env/'firmware.bin' for env in environments}
        if self.speech:
            artifacts.pop('native_intelligence',None)
            artifacts['speech_intelligence']=ROOT/'speech/.pio/build/speech_intelligence/firmware.bin'
            artifacts['speech_models']=ROOT/'speech/.pio/build/speech_intelligence/srmodels/srmodels.bin'
        hashes={}
        for env,artifact in artifacts.items():
            if artifact.is_file(): hashes[env]=hashlib.sha256(artifact.read_bytes()).hexdigest()
            else: errors.append(f'Missing build artifact for hash: {env}')
        setup='Physical output GPIO inhibited; optional PIR; synthetic virtual-house inputs' if self.simulation_only else 'USB-powered bare boards; no sensors or loads'
        data=dict(source='HARDWARE_MEASURED',setup=setup,started_utc=stamp,
                  ended_utc=datetime.now(timezone.utc).isoformat(),
                  firmware_sha256=hashes,cases=self.cases,trials=self.trials,records=self.records,errors=errors)
        stem=datetime.now(timezone.utc).strftime('research_%Y%m%dT%H%M%SZ')
        path=out/(stem+'.json'); path.write_text(json.dumps(data,indent=2)+'\n')
        (out/(stem+'.log')).write_text('\n'.join(self.lines)+'\n')
        print('Evidence:',path,flush=True)
        self.stack.close(); return path

def run(args):
    if args.native or args.speech:
        from scripts.resume_validation import verified_flash_artifact
        targets=[('native','native_voice'),('speech','speech_intelligence')] if args.speech else [('native','native_voice'),('native','native_intelligence')]
        for project,env in targets:
            if not verified_flash_artifact(ROOT/project/'.pio/build'/env/'firmware.bin',(ROOT/'benchmarks/raw/hardware').glob(f'flash_{env}_*.json')):
                raise RuntimeError(f'{env}: current build does not match the last successful flash; deploy before measuring')
    b=Bench([args.node_a,args.node_b],args.native or args.speech,args.simulation_only,args.speech); errors=[]; stamp=datetime.now(timezone.utc).isoformat()
    try:
        b.poll(1)
        if args.simulation_only:
            policy=b.request(b.a,b'P','policy_saved')
            if not policy or policy.get('physical_outputs_enabled') is not False:
                raise RuntimeError('Simulation-only test requires firmware-confirmed GPIO inhibition')
            b.case('physical_outputs_inhibited',True)
        for port in (b.a,b.b):
            r=b.request(port,b't','selftest'); b.case(port+'_runtime_selftest',r and r.get('passed'))
            r=b.request(port,b'm','arena_probe'); b.case(port+'_psram_probe',r and r.get('passed') and r.get('heap_ok'))
        b.poll(5)
        if args.speech:
            response=b.http('GET','/api/speech')
            model=response.get('body',{}) if response else {}
            b.case('esp32_speech_model_ready',response and response.get('status')==200 and model.get('supported') and model.get('phase') in (1,4),json.dumps(model))
        s=b.state(); b.case('authenticated_room_telemetry',s and s.get('known'))
        index=len(b.records); b.connections[b.b].write(b'H')
        r=b.wait(lambda r:r['port']==b.b and r['device'].get('kind')=='http_selftest',35,index)
        b.case('http_loopback_suite',r and r.get('passed'),json.dumps(r))
        for record in b.records[index:]:
            d=record['device']
            if d.get('kind')=='http_case': b.case('http_'+d['case'],d['passed'])
        b.poll(2)
        b.command('away_scene','2 0 1 0')
        b.command('light_on','1 0 1 0')
        b.command('rapid_off_denied','1 0 0 0',1)
        b.poll(3.2); b.command('light_off_after_dwell','1 0 0 0')
        b.command('sleep_scene','2 0 2 0')
        b.poll(3.2); b.command('away_after_sleep','2 0 1 0')
        b.command('enable_fan_automation','4 1 1 0')
        b.command('set_vacancy_grace','4 2 0 30000')
        b.command('reject_short_grace','4 2 0 29999',2)
        b.command('reject_timer_overflow','3 1 0 86400001',2)
        b.command('home_scene','2 0 0 0')
        b.poll(3.2) # Fan was switched off by Away; respect the actual dwell gate.
        b.command('fan_on','1 1 1 0')
        b.command('fan_timer','3 1 0 3000')
        b.poll(1.3); s=b.state(); b.case('fan_reported_on_before_timer',s and s.get('fan') is True)
        b.poll(3.2); s=b.state(); b.case('fan_off_after_timer',s and s.get('fan') is False)
        b.command('timer_cancel','3 1 0 0')
        b.command('resume_light_auto','5 0 0 0')
        b.command('restore_fan_policy','4 1 0 0')
        b.command('restore_light_policy','4 0 1 0')
        b.command('restore_grace','4 2 0 120000')
        b.command('all_off_scene','2 0 3 0')
        # Repeated OFF commands are idempotent. These measure command+ACK RTT,
        # not relay switching or an AI workload. Do not pool with ON/OFF trials.
        for i in range(args.trials):
            b.command(f'noop_off_{i}','1 0 0 0',trial=True)
            if (i+1)%100==0: print(f'RTT progress: {i+1}/{args.trials}',flush=True)
        successful=[t['result']['rtt_us'] for t in b.trials if t['passed']]
        b.case('noop_ack_trials',len(successful)==args.trials,f'{len(successful)}/{args.trials}')
        b.request(b.a,b'd','fault_armed')
        r=b.command('lost_ack_retry','1 0 0 0')
        b.case('retry_uses_cached_result',r and r.get('attempts')==2)
        before=[r['device'] for r in b.records if r['port']==b.a and r['device'].get('kind')=='safety'][-1:]
        b.request(b.b,b'F','injection'); b.poll(5.2)
        after=[r['device'] for r in b.records if r['port']==b.a and r['device'].get('kind')=='safety'][-1:]
        b.case('bad_auth_tag_rejected',bool(before and after and after[0]['rejected']>before[0]['rejected']))
        index=len(b.records); b.request(b.b,b'V','injection'); b.poll(.3)
        changed=[r['device'] for r in b.records[index:] if r['port']==b.a and r['device'].get('kind')=='actuation']
        b.case('changed_duplicate_payload_rejected',any(x.get('status')==3 for x in changed))
        previous=after
        index=len(b.records)
        b.request(b.b,b'R','injection'); b.poll(5.2)
        after=[r['device'] for r in b.records if r['port']==b.a and r['device'].get('kind')=='safety'][-1:]
        b.case('duplicate_replied_without_reexecution',bool(previous and after and after[0]['duplicates']>previous[0]['duplicates'] and after[0]['last_command']==previous[0]['last_command']))
        duplicate=[r['device'] for r in b.records[index:] if r['port']==b.a and r['device'].get('kind')=='duplicate_result']
        b.case('cached_ack_survives_altered_replay',any(x.get('status')==0 for x in duplicate))
        idx=len(b.records); b.request(b.b,b'W','injection'); b.poll(1)
        wrong=[r['device'] for r in b.records[idx:] if r['port']==b.a and r['device'].get('kind')=='actuation']
        b.case('wrong_boot_rejected',any(x.get('status')==4 for x in wrong))
        b.request(b.a,b'O','fault_armed')
        index=len(b.records); b.connections[b.b].write(b'C1 0 0 0\n')
        timeout=b.wait(lambda r:r['port']==b.b and r['device'].get('kind')=='transaction' and r['device'].get('phase')==3,3,index)
        b.case('application_outage_timeout',timeout and timeout.get('outcome_known') is False)
        b.poll(4.5); s=b.state(); b.case('stale_state_becomes_unknown',s and s.get('known') is False,json.dumps(s))
        queued=b.request(b.b,b'C1 0 0 0\n','queued')
        b.case('stale_control_rejected',queued and queued.get('accepted') is False,json.dumps(queued))
        b.poll(3); s=b.state(); b.case('telemetry_recovers_after_outage',s and s.get('known'))
        b.command('control_recovers_after_outage','1 0 0 0')
        remaining=args.soak_seconds
        while remaining>0:
            chunk=min(30,remaining); b.poll(chunk); remaining-=chunk
            print(f'Soak remaining: {remaining:.0f} s',flush=True)
        for port in (b.a,b.b):
            samples=[r['device'] for r in b.records if r['port']==port and r['device'].get('kind')=='memory']
            b.case(port+'_memory_headroom',bool(samples and all(x['memory_config_ok'] and x['internal_free']>=65536 for x in samples)))
            b.case(port+'_bounded_queue',bool(samples and all(x['rx_dropped']==0 for x in samples)))
        for port,task in ((b.a,'ControlTask'),(b.a,'LinkTask'),(b.b,'LinkTask')):
            task_samples=[r['device'] for r in b.records if r['port']==port and r['device'].get('kind')=='rtos' and r['device'].get('task')==task]
            b.case(port+'_'+task+'_progress',bool(len(task_samples)>=2 and task_samples[-1]['jobs']>task_samples[0]['jobs']))
            b.case(port+'_'+task+'_watchdog',bool(task_samples and all(r['watchdog'] for r in task_samples)))
            b.case(port+'_'+task+'_stack_margin',bool(task_samples and min(r['stack_free_bytes'] for r in task_samples)>=1024))
            b.case(port+'_'+task+'_diagnostic_queue',bool(task_samples and all(r['diagnostic_dropped']==0 for r in task_samples)))
        b.connections[b.a].write(b'a'); b.poll(1)
    except Exception as error:
        errors.append(repr(error)); b.case('runner_exception',False,repr(error))
    finally:
        path=b.save(stamp,errors)
    report(path)
    return 1 if errors or any(c['status']=='FAIL' for c in b.cases) else 0

def report(path):
    data=json.loads(path.read_text()); cases=data['cases']; trials=data['trials']
    output=ROOT/'benchmarks/reports'; output.mkdir(parents=True,exist_ok=True)
    with (output/'HARDWARE_TEST_RESULTS.csv').open('w',newline='') as f:
        writer=csv.DictWriter(f,fieldnames=['test','source','status','details']); writer.writeheader(); writer.writerows(cases)
    rows=['# Hardware research test results','',f'Evidence: `{path.relative_to(ROOT)}`. Source: **HARDWARE_MEASURED**.',
          '', data.get('setup','Setup unspecified')+'. HTTP tests use on-board loopback; they do not validate a phone browser or external network load. Virtual-house sensor inputs are SIMULATED; firmware execution is HARDWARE_MEASURED.',
          '', '| Test | Result | Source |','|---|---|---|']
    rows += [f"| {c['test']} | {c['status']} | {c['source']} |" for c in cases]
    success=[t['result']['rtt_us'] for t in trials if t['passed']]
    rows += ['', '## Runtime memory observations','',
             '| Port | Samples | Min internal free B | Min largest internal block B | Min PSRAM free B | Min stack free B | Max RX drops | Max TX enqueue failures |',
             '|---|---:|---:|---:|---:|---:|---:|---:|']
    for port in sorted({r['port'] for r in data['records']}):
        samples=[r['device'] for r in data['records'] if r['port']==port and r['device'].get('kind')=='memory']
        if samples:
            row=[port,len(samples)]+[min(s[k] for s in samples) for k in ('internal_free','internal_largest','psram_free','stack_free_bytes')]+[max(s[k] for s in samples) for k in ('rx_dropped','tx_failed')]
            rows.append('| '+' | '.join(map(str,row))+' |')
    analysis={}
    task_records=[r for r in data['records'] if r['device'].get('kind')=='rtos']
    if task_records:
        rows += ['', '## FreeRTOS task measurements','',
                 '| Port / task | Core | Priority | Period ms | Max execution us | Max release lateness us | Deadline misses | Min stack free B | Watchdog |',
                 '|---|---:|---:|---:|---:|---:|---:|---:|---|']
        for port,task in sorted({(r['port'],r['device']['task']) for r in task_records}):
            samples=[r['device'] for r in task_records if r['port']==port and r['device']['task']==task]
            latest=samples[-1]
            values=[port+' / '+task,latest['core'],latest['priority'],latest['period_ms'],
                    max(s['max_exec_us'] for s in samples),max(s['max_late_us'] for s in samples),max(s['deadline_misses'] for s in samples),
                    min(s['stack_free_bytes'] for s in samples),'registered' if all(s['watchdog'] for s in samples) else 'FAILED']
            rows.append('| '+' | '.join(map(str,values))+' |')
        rows += ['', 'The configured five-millisecond period is a software target. Execution maxima and missed deadlines are observations, not a proven WCET bound. Watchdog registration is verified; an induced watchdog reboot/power-cycle recovery test is still pending.']
    if success:
        analysis=summarize(success); analysis['completion_wilson_ci95']=wilson(len(success),len(trials))
        analysis['capture_recovery_queries']=sum(t.get('capture_recovery_requested',False) for t in trials)
        rows += ['', '## Idempotent OFF command acknowledgement RTT','',
                 'These are independent command IDs issued sequentially in one bench run. The output was already off. Timing includes Node B queueing, authenticated radio request, SafetyKernel processing and acknowledgement; no physical switching or AI inference is measured.',
                 '', '| Metric | Value |','|---|---:|']
        rows += [f'| Submitted trials | {len(trials)} |',f'| Completed with expected outcome | {len(success)} |',f'| Failed / missing outcomes | {len(trials)-len(success)} |']
        rows.append(f"| USB diagnostic recovery queries | {analysis['capture_recovery_queries']} |")
        for key in ('n','mean','median','p95','p99','minimum','maximum'):
            rows.append(f'| {key}'+(' (microseconds)' if key!='n' else '')+f" | {analysis[key]:.3f} |")
        rows += [f"| Bootstrap 95% CI for mean (microseconds) | {analysis['mean_ci95_bootstrap'][0]:.3f} to {analysis['mean_ci95_bootstrap'][1]:.3f} |",
                 f"| Wilson 95% completion interval | {analysis['completion_wilson_ci95'][0]:.5f} to {analysis['completion_wilson_ci95'][1]:.5f} |",
                 '', 'Bootstrap: 2,000 resamples, seed 20261020. The interval assumes exchangeable samples; autocorrelation and one-session bias limit inference. No cross-device/population generalization is established.']
    rows += ['', '## Publication limits','', 'Speech accuracy, occupancy prediction, electrical energy, physical relay behavior, external interference, power-cycle recovery, long-duration stability, and single-vs-dual-model comparisons remain pending. See `docs/PUBLICATION_TEST_MATRIX.md`.']
    (output/'HARDWARE_TEST_RESULTS.md').write_text('\n'.join(rows)+'\n')
    (output/'hardware_statistics.json').write_text(json.dumps(analysis,indent=2)+'\n')
    with (output/'COMMAND_RTT_TRIALS.csv').open('w',newline='') as f:
        writer=csv.writer(f); writer.writerow(['trial','source','passed','rtt_us','attempts','status'])
        for t in trials:
            r=t['result'] or {}; writer.writerow([t['name'],'HARDWARE_MEASURED',t['passed'],r.get('rtt_us'),r.get('attempts'),r.get('status')])

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--node-a',default='COM5'); parser.add_argument('--node-b',default='COM7')
    setup=parser.add_mutually_exclusive_group(required=True)
    setup.add_argument('--bare-boards',action='store_true')
    setup.add_argument('--simulation-only',action='store_true')
    parser.add_argument('--native',action='store_true',help='Hash native ESP-IDF build artifacts')
    parser.add_argument('--speech',action='store_true',help='Node A native, Node B ESP-SR build')
    parser.add_argument('--trials',type=int,default=100); parser.add_argument('--soak-seconds',type=float,default=60)
    args=parser.parse_args()
    if args.trials<1 or args.soak_seconds<0: parser.error('Invalid trial count/duration')
    raise SystemExit(run(args))
