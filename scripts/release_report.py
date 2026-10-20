"""Build evidence tables from an explicitly selected retained hardware session.

Never select the latest file implicitly: incomplete runs must remain visible.
"""
import csv
import hashlib
import json
import xml.etree.ElementTree as ET
from pathlib import Path
from scripts.research_stats import summarize

ROOT=Path(__file__).resolve().parents[1]
TRACE='research_20261020T060226Z.json'
RECOVERY='recovery_20261020T061525Z.json'
SINGLE_B='single_b_20261020T080754Z.json'
AUDIO='audio_20261020T081414Z.json'

def main():
    raw=ROOT/'benchmarks/raw/hardware'; out=ROOT/'benchmarks/reports'
    d=json.loads((raw/TRACE).read_text()); recovery=json.loads((raw/RECOVERY).read_text())
    single=json.loads((raw/SINGLE_B).read_text()); audio=json.loads((raw/AUDIO).read_text())
    records=[r['device'] for r in d['records']]
    cases=d['cases']; success=sum(c['status']=='PASS' for c in cases)
    trials=d['trials']; values=[t['result']['rtt_us'] for t in trials if t['passed']]
    stats=summarize(values)
    staged_b=hashlib.sha256((ROOT/'speech/.pio/build/speech_intelligence/firmware.bin').read_bytes()).hexdigest()
    staged_change=staged_b!=d['firmware_sha256']['speech_intelligence']
    current_b_verified=staged_b==single['firmware_sha256']
    staged_a=hashlib.sha256((ROOT/'native/.pio/build/native_voice/firmware.bin').read_bytes()).hexdigest()
    root=ET.parse(ROOT/'benchmarks/raw/host_tests.xml').getroot()
    suites=[root] if root.tag=='testsuite' else list(root.iter('testsuite'))
    host=sum(int(s.get('tests',0))-int(s.get('failures',0))-int(s.get('errors',0))-int(s.get('skipped',0)) for s in suites)
    rows=[['Host regression','HOST_MEASURED',f'{host} passed','Actual firmware C/C++ tests included'],
          ['Previous paired controller check rows','HARDWARE_MEASURED',f'{success}/{len(cases)} PASS','Earlier image hashes; current paired regression pending'],
          ['Sequential command outcomes','HARDWARE_MEASURED',f'{len(values)}/{len(trials)}','Idempotent OFF; GPIO inhibited'],
          ['Command ACK median / p95 / p99 (ms)','HARDWARE_MEASURED',f'{stats["median"]/1000:.3f} / {stats["p95"]/1000:.3f} / {stats["p99"]/1000:.3f}','Device RTT, not speech/physical actuation latency'],
          ['HTTP loopback','HARDWARE_MEASURED','23/23 PASS','Includes virtual-house input/output/expiry cases'],
          ['Watchdog recovery','HARDWARE_MEASURED',f'{sum(c["status"]=="PASS" for c in recovery["cases"])} recorded checks PASS; campaign incomplete','Reset cause, boot counter and saved policy passed; final restored-policy read interrupted'],
          ['Speech model readiness','HARDWARE_MEASURED','PASS','MultiNet6 English loaded; recognition currently fails'],
          ['Synthetic speech benchmark','SIMULATED input / HARDWARE_MEASURED processing','Completed: 0/16 command matches; 0/2 negative false accepts','All 18 PCM checksums matched; no human accuracy result'],
          ['House interface interaction','HOST_MEASURED','Layout/movement/tour/dropout inspected offline','No board decisions or fabricated outputs in preview'],
          ['Human microphone / acoustic evaluation','NOT_MEASURED','PENDING','Requires returned board and labeled human recordings'],
          ['Physical sensors / relays / energy','NOT_MEASURED','PENDING','Nothing wired; physical outputs inhibited'],
          ['Current Node B campaign','HARDWARE_MEASURED','32/32 rows passed','Shadow monitor/API tests; no actuation'],
          ['Evidence-monitor fault matrix','SIMULATED input / HOST_MEASURED execution','4096 conditions; zero necessary-contract violations','Four eligible cases refused; two physical false assertions under jointly lying sensors'],
          ['B update','HARDWARE_MEASURED' if current_b_verified else 'HOST_MEASURED','Flashed; single-board tested' if current_b_verified else 'Current build differs from selected B test','Transport/provenance/PCM diagnostics and shadow monitor; A remains unavailable']]
    with (out/'PROJECT_COMPLETION_TABLE.csv').open('w',newline='') as f:
        writer=csv.writer(f); writer.writerow(['area','source','result','scope']); writer.writerows(rows)
    memory=[]; timing=[]
    for node in ('A','B'):
        samples=[r for r in records if r.get('kind')=='memory' and r.get('node')==node]
        memory.append([node]+[min(r[k] for r in samples) for k in ('internal_free','internal_min','internal_largest','psram_free','psram_largest')])
    for node,task in [('A','ControlTask'),('A','LinkTask'),('B','LinkTask')]:
        samples=[r for r in records if r.get('kind')=='rtos' and r.get('node')==node and r.get('task')==task]
        timing.append([node,task,max(r['max_exec_us'] for r in samples),max(r['max_late_us'] for r in samples),max(r['deadline_misses'] for r in samples),min(r['stack_free_bytes'] for r in samples)])
    def table(headers,rows):
        return '\n'.join(['| '+' | '.join(headers)+' |','|'+'---|'*len(headers),*['| '+' | '.join(map(str,row))+' |' for row in rows]])
    hashes='\n'.join(f'- `{key}`: `{value}`' for key,value in d['firmware_sha256'].items())
    text=f'''# Current validation summary — 20 October 2026

Node B has returned on COM7; Node A is still unavailable. The current B build passed a single-board shadow/API campaign, while the paired controller timings below belong to the earlier explicitly hashed session. Port 8765 remains the offline house preview; port 8766 is a live byte bridge to B. Neither host service substitutes control or inference for the missing A.

## Completed and incomplete checks

{table(['Area','Source','Result','Scope'],rows)}

## Historical paired controller run

Trace: [raw JSON](../raw/hardware/{TRACE}), [serial log](../raw/hardware/{TRACE.replace('.json','.log')}). Observation lasted approximately 194.6 seconds, including a 60-second soak. One pair of boards, one session; speech model resident but no inference workload during these command trials. No sensor/relay/load was connected.

ACK mean: {stats['mean']/1000:.3f} ms; maximum: {stats['maximum']/1000:.3f} ms. See [individual cases](HARDWARE_TEST_RESULTS.md), [1,000 trials](COMMAND_RTT_TRIALS.csv) and [statistics](hardware_statistics.json) for descriptive bootstrap/Wilson intervals. These do not establish population reliability or a speedup over a controlled baseline.

### RTOS observations — HARDWARE_MEASURED

{table(['Node','Task','Max execution µs','Max release lateness µs','Observed target misses','Min free stack B'],timing)}

Five milliseconds is the design target; observed maxima are not a WCET proof. Wi-Fi/SDK tasks still share CPU time.

### Memory observations — HARDWARE_MEASURED, bytes

{table(['Node','Min sampled internal free','SDK internal low-water since boot','Min largest internal block','Min sampled PSRAM free','Min largest PSRAM block'],memory)}

The SDK low-water measurement includes earlier work in that boot; it is different from the lowest periodically sampled free-memory reading. In particular B reached **55,459 B** internal low-water with the model resident; do not claim a continuously maintained 64 KiB reserve. These measurements do not include a completed simultaneous audio/control stress campaign. Each board has its own 8 MiB PSRAM.

## Recovery and failed attempts

[Recovery raw evidence](../raw/hardware/{RECOVERY}) records a real task-watchdog reset (reason 6), increased persistent boot counter, preserved Away/grace policy, OFF software outputs and resumed acknowledged radio commands. Eleven recorded checks passed. The final original-policy verification was interrupted by a USB transport exception, so the full recovery campaign is **incomplete**. Its old runner returned zero despite this restore exception; that exit-code bug is now fixed. The historical raw file remains unchanged.

The earlier native run `research_20261020T055435Z.json` failed two stale-state checks. A full 64-byte USB packet delayed its fault acknowledgement; a ZLP fix was applied and the current 81-row run passed. Earlier export/uploader failures, journals and audio transport timeouts are retained. No unsuccessful attempt is pooled into a passing latency dataset.

The earlier audio timeout was reproduced on returned B. A 20 ms diagnostic send budget could truncate JSON while SpeechTask preempted diagnostics. A bounded progress/total budget fixed the observed transport failure. B now checksums PCM before inference. The latest [18-input speech run](../raw/hardware/{AUDIO}) completed without runner errors and with matching input checksums, but **detected none of the 16 command clips**. Silence/noise returned no command. This is a failed functional smoke result, not human accuracy or false-accepts/hour. All previous failures remain retained.

## Current B shadow evidence

[Current raw B trace](../raw/hardware/{SINGLE_B}) passed 32 rows, including all 23 monitor cases. One case sweeps 1024 combinations. B reports a 60 B monitor object and 375 us for that aggregate self-test; this is not per-transition timing or WCET. The monitor is not yet integrated into A's output registry, and synthetic feedback never becomes physical verification. See the [deep-review response](../../docs/DEEP_REVIEW_RESPONSE.md), [novelty dossier](../../docs/NOVELTY_CASE.md) and [ablation CSV](EVIDENCE_ABLATIONS.csv).

## Tested firmware/model SHA256

{hashes}

## Resume with both bare boards

```powershell
python scripts/resume_validation.py --deploy-staged
```

The current B image SHA256 is `{staged_b}`; it is flashed and matches the selected single-board trace: `{current_b_verified}`. Its static linker use is 84,168 B RAM and 1,055,237 B application flash. Current A compiled with SHA256 `{staged_a}` but is not flashed because A is absent; static linker use is 63,128 B RAM and 649,077 B application flash. These are HOST_MEASURED build sizes, not runtime heap peaks. The historical paired hashes above remain unchanged.

The resume command requires both boards and deploys reviewed A and B images/model with MAC verification and preserved NVS, then runs GPIO-inhibited paired controller/recovery and synthetic speech suites. It does not enable outputs or modify the phone. Without --deploy-staged it refuses a build/last-flash mismatch. Speech currently fails recognition, so expect a nonzero audio result until that defect is repaired. Hardware-dependent and publication-dependent gates remain listed in [the publication matrix](../../docs/PUBLICATION_TEST_MATRIX.md).
'''
    (out/'PROJECT_VALIDATION_SUMMARY.md').write_text(text,encoding='utf-8')
    payload=dict(trace=TRACE,recovery=RECOVERY,single_b_trace=SINGLE_B,audio_trace=AUDIO,host_passed=host,cases_passed=success,cases_total=len(cases),command_trials=len(trials),command_success=len(values),statistics_us=stats,memory_bytes=memory,rtos=timing,firmware_sha256=d['firmware_sha256'],current_b_sha256=staged_b,current_b_single_board_verified=current_b_verified,staged_a_sha256=staged_a,hardware_available='B_ONLY',speech_campaign_complete=True,speech_functional=False,recovery_campaign_complete=False)
    (out/'CURRENT_EVIDENCE.json').write_text(json.dumps(payload,indent=2)+'\n')
    print('Wrote current validation summary, completion CSV and evidence JSON.')

if __name__=='__main__': main()
