"""Create unique bench credentials once; never print secrets or rotate implicitly."""
import json
import secrets
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def provision():
    path = ROOT / 'local_credentials.json'
    header = ROOT / 'firmware/common/config/private_config.h'
    if path.exists():
        config = json.loads(path.read_text())
    else:
        config = dict(ap_ssid='ClawGrid-Assistant', ap_password=secrets.token_urlsafe(15),
                      api_token=secrets.token_hex(24), pmk=secrets.token_hex(16),
                      lmk=secrets.token_hex(16), hmac_key=secrets.token_hex(32),
                      node_a_mac='e0:72:a1:f6:a9:68', node_b_mac='e0:72:a1:f5:bc:b0')
        path.write_text(json.dumps(config, indent=2)+'\n')
    def array(name, value):
        return f'static const uint8_t {name}[] = {{'+','.join(str(b) for b in bytes.fromhex(value.replace(':','')))+'};\n'
    source = '#pragma once\n#include <stdint.h>\n'
    for key in ('ap_ssid','ap_password','api_token'):
        source += f'static const char k{key.title().replace("_", "")}[] = {json.dumps(config[key])};\n'
    for name, key in [('kPmk','pmk'),('kLmk','lmk'),('kHmacKey','hmac_key'),('kNodeA','node_a_mac'),('kNodeB','node_b_mac')]:
        source += array(name, config[key])
    header.write_text(source)
    print('Local credentials and private build header ready. Secrets are in local_credentials.json; excluded from version control.')

if __name__ == '__main__':
    provision()
