"""
Hardware Detection Script for ClawGrid V2.0
Scans available serial ports and queries chip information via esptool.
Strictly records verified facts without assumptions.
"""

import sys
import subprocess
import serial.tools.list_ports

def scan_ports():
    ports = list(serial.tools.list_ports.comports())
    print(f"[Detector] Found {len(ports)} serial port(s):")
    for p in ports:
        print(f"  - {p.device}: {p.description} [{p.hwid}]")
    return [p.device for p in ports]

def probe_esp(port):
    print(f"\n[Detector] Probing {port} with esptool...")
    try:
        cmd = [sys.executable, "-m", "esptool", "--port", port, "chip-id"]
        res = subprocess.run(cmd, capture_output=True, text=True, timeout=10)
        print(res.stdout)
        
        cmd_flash = [sys.executable, "-m", "esptool", "--port", port, "flash-id"]
        res_flash = subprocess.run(cmd_flash, capture_output=True, text=True, timeout=10)
        print(res_flash.stdout)
    except Exception as e:
        print(f"[Detector:ERROR] Failed to query {port}: {e}")

def main():
    print("========================================")
    print("  ClawGrid V2.0 Hardware Auto-Discovery ")
    print("========================================")
    ports = scan_ports()
    for port in ports:
        probe_esp(port)

if __name__ == "__main__":
    main()
