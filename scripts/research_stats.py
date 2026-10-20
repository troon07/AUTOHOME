"""Small dependency-free descriptive statistics; no synthetic observations."""
import math
import random
import statistics

def percentile(values, p):
    if not values or not 0 <= p <= 100:
        raise ValueError('Nonempty data and percentile 0..100 required')
    ordered=sorted(values)
    position=(len(ordered)-1)*p/100
    low=math.floor(position); high=math.ceil(position)
    return ordered[low]+(ordered[high]-ordered[low])*(position-low)

def summarize(values, seed=20261020):
    if not values or any(not math.isfinite(x) for x in values):
        raise ValueError('Finite nonempty observations required')
    rng=random.Random(seed); n=len(values)
    means=[statistics.fmean(rng.choices(values,k=n)) for _ in range(2000)]
    return dict(n=n,mean=statistics.fmean(values),median=statistics.median(values),
                p95=percentile(values,95),p99=percentile(values,99),
                minimum=min(values),maximum=max(values),
                mean_ci95_bootstrap=[percentile(means,2.5),percentile(means,97.5)],
                bootstrap_seed=seed,bootstrap_resamples=2000)

def wilson(successes,total):
    if total<=0 or not 0<=successes<=total:
        raise ValueError('Valid binomial counts required')
    z=1.959963984540054; p=successes/total; denominator=1+z*z/total
    centre=(p+z*z/(2*total))/denominator
    radius=z*math.sqrt(p*(1-p)/total+z*z/(4*total*total))/denominator
    return [max(0,centre-radius),min(1,centre+radius)]
