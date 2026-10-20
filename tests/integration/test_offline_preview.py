"""Ensure an offline UI cannot masquerade as a functioning MCU controller."""
import socket
import subprocess
import sys
import time
import urllib.error
import urllib.request
from pathlib import Path
import pytest

ROOT=Path(__file__).resolve().parents[2]

@pytest.fixture(scope='module')
def preview():
    with socket.socket() as listener:
        listener.bind(('127.0.0.1',0)); port=listener.getsockname()[1]
    child=subprocess.Popen([sys.executable,'scripts/browser_bridge.py','--preview-only','--port',str(port)],cwd=ROOT,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
    base=f'http://127.0.0.1:{port}'
    try:
        for _ in range(50):
            try:
                with urllib.request.urlopen(base+'/simulation',timeout=.2): break
            except OSError: time.sleep(.02)
        else: raise RuntimeError('Preview did not start')
        yield base
    finally:
        child.terminate(); child.wait(timeout=5)

def test_offline_page_declares_absent_processing(preview):
    with urllib.request.urlopen(preview+'/simulation') as response:
        page=response.read().decode()
        assert 'OFFLINE LAYOUT PREVIEW' in page
        assert 'ESP32 inference, decisions and device outputs are unavailable' in page
        assert response.headers['Cache-Control']=='no-store'

def test_offline_control_cannot_report_success(preview):
    for path,body in [('/api/simulation',None),('/api/control',b'6 0 1 0'),('/api/speech/audio',bytes(3200))]:
        request=urllib.request.Request(preview+path,data=body)
        with pytest.raises(urllib.error.HTTPError) as failed: urllib.request.urlopen(request)
        assert failed.value.code==503
        assert b'"known":false' in failed.value.read()

def test_preview_rejects_foreign_origin(preview):
    request=urllib.request.Request(preview+'/api/control',data=b'6 0 1 0',headers={'Origin':'https://foreign.example'})
    with pytest.raises(urllib.error.HTTPError) as failed: urllib.request.urlopen(request)
    assert failed.value.code==403
