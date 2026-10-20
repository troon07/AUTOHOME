"""Induce a task-watchdog reboot only on GPIO-inhibited firmware.

Changes and restores the original policy, records real NVS recovery and boot
counter behavior. No power-cut, brownout, physical relay or signed OTA claim.
"""
import argparse
import csv
import json
import sys
import time
from datetime import datetime,timezone
from pathlib import Path
import serial

ROOT=Path(__file__).resolve().parents[1]; sys.path.insert(0,str(ROOT))
from scripts.research_hil import Bench


def run(args):
    b=Bench([args.node_a,args.node_b],native=True,simulation_only=True)
    original=None; errors=[]
    try:
        b.poll(.5); original=b.request(b.a,b'P','policy_saved')
        if not original or original.get('physical_outputs_enabled') is not False:
            raise RuntimeError('Firmware did not confirm GPIO inhibition')
        b.case('recovery_gpio_inhibited',True)
        b.command('recovery_away','2 0 1 0')
        b.command('recovery_custom_grace','4 2 0 45000')
        b.poll(6); before=b.request(b.a,b'P','policy_saved')
        b.case('policy_committed_before_reset',before and before.get('away') is True and before.get('grace_ms')==45000)
        if not before or before.get('grace_ms')!=45000: raise RuntimeError('Policy commit missing; reboot not attempted')
        armed=b.request(b.a,b'X','watchdog_injection')
        if not armed or not armed.get('armed'): raise RuntimeError('Watchdog injection did not arm')
        # USB may re-enumerate after the reset. Reopen only the transport;
        # no actuator command is automatically retried during recovery.
        try: b.poll(9)
        except serial.SerialException as e:
            errors.append('USB re-enumeration: '+str(e))
            connection=b.connections[b.a]; connection.close(); time.sleep(1)
            connection.open(); b.buffers[b.a].clear()
        after=b.request(b.a,b'P','policy_saved',timeout=5)
        b.case('watchdog_boot_counter_advanced',bool(after and after['boot']>before['boot']))
        b.case('task_watchdog_reset_reason',bool(after and after.get('reset_reason')==6))
        b.case('nvs_policy_survived_watchdog',bool(after and after.get('away') is True and after.get('grace_ms')==45000))
        b.poll(2); state=b.state()
        b.case('offline_outputs_after_recovery',bool(state and state.get('known') and not state.get('light') and not state.get('fan') and state.get('mode')==1))
        b.command('radio_command_after_watchdog','1 0 0 0')
    except Exception as e:
        errors.append(repr(e)); b.case('recovery_runner_exception',False,repr(e))
    finally:
        if original:
            try:
                b.command('restore_original_grace',f'4 2 0 {original["grace_ms"]}')
                b.command('restore_original_scene',f'2 0 {1 if original["away"] else 0} 0')
                b.poll(6); restored=b.request(b.a,b'P','policy_saved')
                b.case('original_policy_restored',bool(restored and restored['grace_ms']==original['grace_ms'] and restored['away']==original['away']))
            except Exception as e:
                errors.append('Policy restore: '+repr(e))
                b.case('original_policy_restored',False,'Verification interrupted; inspect saved errors and reconnect boards')
        b.stack.close()
        stamp=datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ')
        path=ROOT/f'benchmarks/raw/hardware/recovery_{stamp}.json'
        path.write_text(json.dumps(dict(source='HARDWARE_MEASURED',setup='GPIO-inhibited, USB-powered ESP32-S3 pair',cases=b.cases,records=b.records,errors=errors),indent=2)+'\n')
        path.with_suffix('.log').write_text('\n'.join(b.lines)+'\n')
        output=ROOT/'benchmarks/reports/RECOVERY_TEST_RESULTS.csv'
        with output.open('w',newline='') as f:
            writer=csv.DictWriter(f,fieldnames=['test','source','status','details']); writer.writeheader(); writer.writerows(b.cases)
        print('Evidence:',path)
    return 1 if errors or any(c['status']=='FAIL' for c in b.cases) else 0


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__); p.add_argument('--node-a',default='COM5'); p.add_argument('--node-b',default='COM7')
    raise SystemExit(run(p.parse_args()))
