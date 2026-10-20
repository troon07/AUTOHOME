import csv
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

def test_partition_boundaries_and_ota_alignment():
    lines = [line for line in (ROOT/"partitions_n16r8.csv").read_text().splitlines() if line and not line.startswith("#")]
    end = 0x9000
    sizes = {}
    for row in csv.reader(lines):
        name, kind, subtype, offset, size, *_ = [x.strip() for x in row]
        start, length = int(offset,0), int(size,0)
        assert start == end, f"Gap or overlap before {name}"
        assert length > 0 and start % 4096 == 0
        if kind == "app":
            assert start % 65536 == 0
        end = start + length
        sizes[name] = length
    assert end == 16*1024*1024
    assert sizes["app0"] == sizes["app1"] == 3*1024*1024
    assert sizes["model"] == 6*1024*1024
    board = json.loads((ROOT/"boards"/"clawgrid_s3_n16r8.json").read_text())
    assert board["upload"]["flash_size"] == "16MB"
    assert board["build"]["arduino"]["memory_type"] == "qio_opi"
