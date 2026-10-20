"""Execute the actual firmware policy, safety and wire code on the host."""
import json
import subprocess
import sys
from pathlib import Path
import pytest

ROOT=Path(__file__).resolve().parents[2]


@pytest.fixture(scope='module')
def executable(tmp_path_factory):
    path=tmp_path_factory.mktemp('firmware_cpp')/'checks.exe'
    common=ROOT/'firmware/common'
    command=[sys.executable,'-m','ziglang','c++','-std=c++17','-O2']
    command += [f'-I{common/name}' for name in ('runtime','protocol','safety_kernel')]
    objects=[]
    for source in ('safety_kernel/safety_kernel.c','safety_kernel/safety_selftest.c','protocol/clawgrid_protocol.c'):
        obj=path.parent/(Path(source).stem+'.o'); objects.append(str(obj))
        result=subprocess.run([sys.executable,'-m','ziglang','cc','-std=c11','-O2',f'-I{common/"safety_kernel"}',f'-I{common/"protocol"}','-c',str(common/source),'-o',str(obj)],capture_output=True,text=True,timeout=180)
        assert result.returncode==0,result.stderr
    command += [str(ROOT/'tests/native/firmware_checks.cpp'),str(common/'runtime/runtime_selftest.cpp'),*objects,'-o',str(path)]
    result=subprocess.run(command,capture_output=True,text=True,timeout=180)
    assert result.returncode==0,result.stderr
    return path


@pytest.mark.parametrize('case',['runtime','safety','automation','house','leases','sensor_boundaries']+[f'evidence_{i}' for i in range(23)])
def test_actual_firmware(executable,case):
    result=subprocess.run([str(executable),case],capture_output=True,text=True,timeout=5)
    assert result.returncode==0,result.stdout+result.stderr
    assert json.loads(result.stdout)==dict(source='HOST_MEASURED',case=case,passed=True)
