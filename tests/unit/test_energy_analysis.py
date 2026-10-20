import pytest
from scripts.energy_analysis import integrate

def test_known_dc_integrals_and_irregular_timestamps():
    assert integrate([(0,5,1),(.1,5,1),(.4,5,1)],.31)['energy_j']==pytest.approx(2)
    assert integrate([(0,5,0),(2,5,1)],2)['energy_j']==pytest.approx(5)

@pytest.mark.parametrize('samples,gap',[
    ([],1), ([(0,5,1)],1), ([(0,5,1),(0,5,1)],1),
    ([(1,5,1),(0,5,1)],1), ([(0,5,1),(2,5,1)],1),
    ([(0,5,1),(1,float('nan'),1)],1), ([(0,5,1),(1,5,-1)],1)])
def test_invalid_or_unobserved_energy_cannot_be_reported(samples,gap):
    with pytest.raises(ValueError): integrate(samples,gap)
