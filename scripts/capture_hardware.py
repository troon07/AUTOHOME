"""Collect real UART telemetry without resetting the boards or inventing metrics."""
import argparse
import hashlib
import json
import time
from contextlib import ExitStack
from datetime import datetime, timezone
from pathlib import Path

import serial

ROOT = Path(__file__).resolve().parents[1]

def capture(ports, seconds, probe=False):
    if seconds <= 0:
        raise ValueError("seconds must be positive")
    started = datetime.now(timezone.utc)
    records, lines = [], []
    buffers = {p: bytearray() for p in ports}
    with ExitStack() as stack:
        connections = {}
        for port in ports:
            connection = serial.Serial(port=None, baudrate=115200, timeout=0, write_timeout=1)
            connection.dtr = False
            connection.rts = False
            connection.port = port
            connection.open()
            stack.callback(connection.close)
            connections[port] = connection
        for connection in connections.values():
            connection.write(b"t" + (b"m" if probe else b""))
        deadline = time.monotonic() + seconds
        while time.monotonic() < deadline:
            for port, connection in connections.items():
                data = connection.read(min(connection.in_waiting, 4096))
                buffers[port].extend(data)
                while b"\n" in buffers[port]:
                    raw, _, rest = buffers[port].partition(b"\n")
                    buffers[port] = bytearray(rest)
                    line = raw.decode("utf-8", errors="replace").strip()
                    if not line:
                        continue
                    lines.append(f"{port} {line}")
                    try:
                        value = json.loads(line)
                    except json.JSONDecodeError:
                        continue
                    if not isinstance(value, dict):
                        continue
                    records.append({"port": port, "capture_monotonic_ns": time.monotonic_ns(), "device": value})
                if len(buffers[port]) > 8192:
                    raise RuntimeError(f"Oversized serial line on {port}")
            time.sleep(0.01)
    artifacts = {}
    for env in ("node_voice", "node_intelligence"):
        p = ROOT / ".pio" / "build" / env / "firmware.bin"
        if p.exists():
            artifacts[env] = hashlib.sha256(p.read_bytes()).hexdigest()
    result = {"source": "HARDWARE_MEASURED", "started_utc": started.isoformat(),
              "duration_seconds_requested": seconds, "ports": ports,
              "firmware_sha256": artifacts, "records": records}
    out = ROOT / "benchmarks" / "raw" / "hardware"
    out.mkdir(parents=True, exist_ok=True)
    stem = started.strftime("%Y%m%dT%H%M%SZ")
    (out / f"{stem}.json").write_text(json.dumps(result, indent=2), encoding="utf-8")
    (out / f"{stem}.log").write_text("\n".join(lines)+"\n", encoding="utf-8")
    print(f"HARDWARE_MEASURED: {len(records)} records saved to {out / (stem+'.json')}")
    for port in ports:
        samples = [r["device"] for r in records if r["port"] == port and r["device"].get("kind") == "memory"]
        if not samples:
            print(f"{port}: NO MEMORY SAMPLES; hardware behavior not verified")
            continue
        print(f"{port}: {len(samples)} samples; internal free minimum {min(s['internal_free'] for s in samples)} B; "
              f"largest block minimum {min(s['internal_largest'] for s in samples)} B; "
              f"PSRAM total {samples[-1]['psram_total']} B")
    failures = []
    for port in ports:
        device_records = [r["device"] for r in records if r["port"] == port]
        for kind in ("memory", "selftest") + (("arena_probe",) if probe else ()):
            selected = [r for r in device_records if r.get("kind") == kind]
            if not selected:
                failures.append(f"{port}: missing {kind}")
            elif kind == "memory" and not all(r.get("memory_config_ok") is True for r in selected):
                failures.append(f"{port}: memory configuration check failed")
            elif kind != "memory" and not all(r.get("passed") is True for r in selected):
                failures.append(f"{port}: {kind} failed")
            if kind == "arena_probe" and any(r.get("heap_ok") is not True for r in selected):
                failures.append(f"{port}: heap integrity failed")
    if failures:
        raise RuntimeError("Capture saved, but verification failed: " + "; ".join(failures))
    return result

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ports", nargs="+", required=True)
    parser.add_argument("--seconds", type=float, default=30)
    parser.add_argument("--probe", action="store_true", help="Request a temporary 256 KiB PSRAM arena test")
    args = parser.parse_args()
    capture(args.ports, args.seconds, args.probe)
