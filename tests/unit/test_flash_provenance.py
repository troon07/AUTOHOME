import hashlib
import json
import pytest
from scripts.resume_validation import verified_flash_artifact

@pytest.mark.parametrize('latest_status,changed,expected',[(0,False,True),(1,False,False),(0,True,False)])
def test_only_last_successful_matching_flash_can_identify_measurements(tmp_path,latest_status,changed,expected):
    image=tmp_path/'firmware.bin'; image.write_bytes(b'reviewed image')
    digest=hashlib.sha256(image.read_bytes()).hexdigest()
    first=tmp_path/'flash_1.json'; last=tmp_path/'flash_2.json'
    record=dict(exit_code=0,artifacts=[dict(address='0x10000',sha256=digest)])
    first.write_text(json.dumps(record)); record['exit_code']=latest_status; last.write_text(json.dumps(record))
    if changed: image.write_bytes(b'new build not flashed')
    assert verified_flash_artifact(image,[first,last]) is expected
