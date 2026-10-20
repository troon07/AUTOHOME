"""Generate standalone figures from a saved real-board trace, without invented data."""
import argparse
import json
from pathlib import Path
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

ROOT=Path(__file__).resolve().parents[1]

def plot(path):
    data=json.loads(path.read_text())
    if data.get('source')!='HARDWARE_MEASURED': raise ValueError('Hardware trace required')
    trials=data.get('trials',[])
    pairs=[(i,t['result']['rtt_us']/1000) for i,t in enumerate(trials) if t.get('passed')]
    if not pairs: raise ValueError('No completed trials to plot')
    fig,axes=plt.subplots(2,2,figsize=(11,7),layout='constrained')
    indices,latencies=zip(*pairs); ordered=sorted(latencies); n=len(ordered)
    axes[0,0].step(ordered,[(i+1)/n for i in range(n)],where='post',color='#007f73')
    axes[0,0].set(xlabel='Acknowledgement RTT (ms)',ylabel='Empirical CDF',title=f'Completed idempotent OFF commands ({n}/{len(trials)})',ylim=(0,1.02))
    axes[0,1].plot(indices,latencies,'.',markersize=2,color='#285b9c')
    axes[0,1].set(xlabel='Sequential trial index',ylabel='Acknowledgement RTT (ms)',title='Within-session timing variability')
    records=data['records']; origin=min(r['host_monotonic_ns'] for r in records)
    for port in sorted({r['port'] for r in records}):
        samples=[r for r in records if r['port']==port and r['device'].get('kind')=='memory']
        elapsed=[(r['host_monotonic_ns']-origin)/1e9 for r in samples]
        for ax,key in [(axes[1,0],'internal_free'),(axes[1,1],'internal_largest')]:
            ax.plot(elapsed,[r['device'][key]/1024 for r in samples],'.-',markersize=2,label=port)
    axes[1,0].set(xlabel='Host observation time (s)',ylabel='Internal free (KiB)',title='Sampled internal memory')
    axes[1,1].set(xlabel='Host observation time (s)',ylabel='Largest internal block (KiB)',title='Contiguous allocation headroom')
    for ax in axes.flat: ax.grid(alpha=.25)
    axes[1,0].legend(); axes[1,1].legend()
    fig.suptitle('HARDWARE_MEASURED · GPIO inhibited · command workload, no speech inference or physical loads\n'+path.name,fontsize=11)
    out=ROOT/'benchmarks/reports/figures'; out.mkdir(parents=True,exist_ok=True)
    for extension in ('png','svg'): fig.savefig(out/('hardware_runtime.'+extension),dpi=240)
    plt.close(fig); print('Figures:',out)

if __name__=='__main__':
    parser=argparse.ArgumentParser(); parser.add_argument('trace',type=Path)
    plot(parser.parse_args().trace)
