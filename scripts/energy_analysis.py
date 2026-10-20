"""Integrate externally supplied DC traces; never synthesize measured energy."""
import argparse
import csv
import hashlib
import json
import math
from pathlib import Path

def integrate(samples,max_gap_s):
    if len(samples)<2 or not math.isfinite(max_gap_s) or max_gap_s<=0:
        raise ValueError('At least two samples and a positive gap bound required')
    energy=0.; largest_gap=0.; previous=None; powers=[]
    for time_s,voltage_v,current_a in samples:
        if not all(math.isfinite(v) for v in (time_s,voltage_v,current_a)) or voltage_v<0 or current_a<0:
            raise ValueError('Finite nonnegative DC voltage/current required')
        power=voltage_v*current_a; powers.append(power)
        if previous is not None:
            dt=time_s-previous[0]
            if dt<=0 or dt>max_gap_s: raise ValueError('Timestamp reversal, duplicate or unmeasured gap')
            energy+=(previous[1]+power)*dt/2; largest_gap=max(largest_gap,dt)
        previous=(time_s,power)
    duration=samples[-1][0]-samples[0][0]
    return dict(samples=len(samples),duration_s=duration,energy_j=energy,average_w=energy/duration,
                observed_peak_w=max(powers),largest_sample_gap_s=largest_gap,
                method='Trapezoidal V(t)*I(t) integration; sampling/calibration errors not estimated')

def main(args):
    metadata=json.loads(args.metadata.read_text(encoding='utf-8'))
    source=metadata.get('measurement_source')
    if source not in ('SIMULATED','HARDWARE_MEASURED'): raise ValueError('Trace source must be explicit')
    if source=='HARDWARE_MEASURED' and not all(metadata.get(k) for k in ('instrument','calibration_reference','captured_utc')):
        raise ValueError('Hardware trace needs instrument, calibration reference and capture time')
    with args.input.open(newline='') as f:
        rows=list(csv.DictReader(f))
    samples=[tuple(float(r[k]) for k in ('time_s','voltage_v','current_a')) for r in rows]
    result=integrate(samples,args.max_gap)
    result.update(input_source=source,analysis_source='HOST_MEASURED',input_sha256=hashlib.sha256(args.input.read_bytes()).hexdigest(),metadata=metadata)
    args.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print('Wrote',args.output)

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input',type=Path,required=True); parser.add_argument('--metadata',type=Path,required=True)
    parser.add_argument('--max-gap',type=float,required=True,help='Maximum admissible inter-sample seconds')
    parser.add_argument('--output',type=Path,required=True)
    main(parser.parse_args())
