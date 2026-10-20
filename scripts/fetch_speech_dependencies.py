"""Fetch the ESP32-S3 subset of official ESP-SR v1.7.0, pinned by commit.

Every file is verified against the Git blob ID in that commit. ESP-SR 1.7.0
declares ESP-IDF >=4.4 compatibility. Keep upstream license files with binaries.
"""
import hashlib
import json
import urllib.request
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
COMMIT='07b0a4d0f6b5de755874d5405150ee0a8eec9865'
BASE=f'https://raw.githubusercontent.com/espressif/esp-sr/{COMMIT}/'
DEST=ROOT/'speech/components/esp-sr'


def selected(path):
    return path in ('CMakeLists.txt','Kconfig.projbuild','idf_component.yml','LICENSE','README.md') or any(
        path.startswith(prefix) for prefix in (
            'src/', 'include/esp32s3/', 'lib/esp32s3/',
            'esp-tts/esp_tts_chinese/include/', 'esp-tts/esp_tts_chinese/esp32s3/',
            'model/multinet_model/mn6_en/',
            'model/multinet_model/fst/',
        )
    ) or (path.startswith('model/') and path.endswith('.py')) or 'LICENSE' in path.upper()


def fetch(entry):
    relative=entry['path']; destination=DEST/relative
    if not destination.resolve().is_relative_to(DEST.resolve()): raise ValueError('Invalid upstream path')
    content=destination.read_bytes() if destination.exists() else urllib.request.urlopen(BASE+relative,timeout=120).read()
    blob=hashlib.sha1(f'blob {len(content)}\0'.encode()+content).hexdigest()
    if blob!=entry['sha']: raise ValueError(f'Git blob mismatch: {relative}')
    destination.parent.mkdir(parents=True,exist_ok=True)
    if not destination.exists(): destination.write_bytes(content)
    return dict(path=relative,git_blob=blob,sha256=hashlib.sha256(content).hexdigest(),bytes=len(content))


if __name__=='__main__':
    tree=json.load(urllib.request.urlopen(f'https://api.github.com/repos/espressif/esp-sr/git/trees/{COMMIT}?recursive=1',timeout=30))
    if tree.get('truncated'): raise RuntimeError('Truncated upstream tree')
    entries=[e for e in tree['tree'] if e['type']=='blob' and selected(e['path'])]
    with ThreadPoolExecutor(max_workers=4) as pool: records=list(pool.map(fetch,entries))
    manifest=dict(upstream='https://github.com/espressif/esp-sr',version='1.7.0',commit=COMMIT,files=records)
    (ROOT/'speech/dependencies_manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print(f'Verified {len(records)} files, {sum(r["bytes"] for r in records)} bytes; ESP-SR {COMMIT}')
