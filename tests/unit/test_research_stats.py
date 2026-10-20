import pytest
from scripts.research_stats import percentile, summarize, wilson

def test_interpolated_quantiles_and_constant_interval():
    assert percentile([40,10,30,20],50)==25
    result=summarize([7]*5)
    assert result['mean_ci95_bootstrap']==[7,7]
    assert result['p99']==7

def test_reliability_interval_never_claims_certainty():
    lower,upper=wilson(100,100)
    assert 0.96<lower<0.97 and upper==pytest.approx(1)
    lower,upper=wilson(0,100)
    assert lower==pytest.approx(0) and 0.03<upper<0.04

def test_invalid_observations_are_rejected():
    for values in ([],[float('nan')],[float('inf')]):
        with pytest.raises(ValueError): summarize(values)
    with pytest.raises(ValueError): wilson(2,1)
