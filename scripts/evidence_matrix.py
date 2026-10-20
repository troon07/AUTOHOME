"""Finite fault/ablation matrix executing the actual C++ observation monitor.

This is a HOST_MEASURED software experiment on SIMULATED evidence. It is not a
real-world safety demonstration or a proof of novelty. Deliberately weak
ablation baselines expose which contract checks matter, not SOTA superiority.
"""
import csv
import hashlib
import json
import subprocess
import sys
from datetime import datetime,timezone
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]

def main():
    build=ROOT/'benchmarks/.host_build'; build.mkdir(exist_ok=True)
    executable=build/'evidence_matrix.exe'
    command=[sys.executable,'-m','ziglang','c++','-std=c++17','-O2',f'-I{ROOT/"firmware/common/runtime"}',str(ROOT/'tests/native/evidence_matrix.cpp'),'-o',str(executable)]
    subprocess.run(command,check=True,capture_output=True,text=True,timeout=180)
    run=subprocess.run([str(executable)],capture_output=True,text=True,timeout=15)
    stamp=datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ')
    output=ROOT/f'benchmarks/raw/evidence_matrix_{stamp}.csv'; output.write_text(run.stdout)
    rows=list(csv.DictReader(run.stdout.splitlines())); summary=[]
    for key in ['ack_only','feedback_only','fresh_bound_feedback','monitor_verified']:
        approved=[r for r in rows if r[key]=='1']
        summary.append(dict(condition=key,input_source='SIMULATED',execution_source='HOST_MEASURED',cases=len(rows),verified_assertions=len(approved),contract_violations=sum(r['contract_oracle']=='0' for r in approved),eligible_cases_not_verified=sum(r['contract_oracle']=='1' and r[key]=='0' for r in rows),physical_false_assertions_in_modeled_lying_sensor_cases=sum(r['actual_on']=='0' for r in approved)))
    report=ROOT/'benchmarks/reports/EVIDENCE_ABLATIONS.csv'
    with report.open('w',newline='') as f:
        writer=csv.DictWriter(f,fieldnames=list(summary[0])); writer.writeheader(); writer.writerows(summary)
    metadata=dict(input_source='SIMULATED',execution_source='HOST_MEASURED',raw_csv=str(output.relative_to(ROOT)),rows=len(rows),conditions=summary,
                  algorithm_sha256=hashlib.sha256((ROOT/'firmware/common/runtime/evidence_monitor.h').read_bytes()).hexdigest(),
                  limitations='Finite designed truth table, not independent real-world data. Joint sensor lies can satisfy every observable contract. No state-of-art baseline, physical safety proof, energy result or novelty established.')
    output.with_suffix('.json').write_text(json.dumps(metadata,indent=2)+'\n')
    print(json.dumps(metadata,indent=2)); return run.returncode

if __name__=='__main__': raise SystemExit(main())
