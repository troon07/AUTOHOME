# Validation findings retained in raw evidence

20 October 2026. This records failed tests and corrections, rather than discarding runs until a clean report exists.

| Raw artifact / stage | Observation | Correction / interpretation |
|---|---|---|
| `20261020T003930Z.json` | Both PSRAM arena probes failed capacity validation although external RAM was initialized | Use SDK physical PSRAM capacity; Arduino heap-accounted size excludes overhead. Corrected probe passed in `20261020T004141Z.json`. |
| `research_20261020T032052Z.json` | Fan ON denied after Away, before dwell expired; follow-up fan-on observation failed | Correct the test sequence to respect dwell. The denial itself was expected SafetyKernel behavior. |
| Same run | Five Node B RX drops appeared during the USB-triggered HTTP loopback diagnostic | Service the radio queue and control state while diagnostic HTTP waits run. Normal HTTP handling already has a separate task. |
| `research_20261020T033115Z.json` | 996 of 1,000 trial records met the host's three-second collection window | Raw records later contain all four missing ACKs with device RTT around 6 ms. USB diagnostic delivery was delayed. The runner now issues a read-only state query on delayed diagnostic delivery, retaining a recovery-query count and the device's RTT. |
| Code review during extended validation | Changed same-ID replay could replace the cached ACK with a replay denial | Only a newly committed command can update the cached outcome; add changed-replay-then-exact-retry regression. |
| Same review | ACK ID/boot did not bind its exact requested operation | Echo all 16 command bytes in the authenticated ACK and compare them before accepting an outcome. |
| Initial concurrent build attempt | PlatformIO shared bookkeeping collision removed an in-progress environment's build state | Run all firmware builds/uploads sequentially. This was a build-tool collision, not a board measurement. |

The final firmware is identified by hashes in the latest hardware trace. Earlier images and host script behavior may differ. Do not pool timings across these revisions without accounting for the configuration change. Read the generated report for final pass/fail outcomes.

## Native/simulation update — 20 October 2026

Native ESP-IDF A and ESP-SR B replaced the Arduino runtime for the current measured configuration. Final retained controller session `research_20261020T060226Z.json` passed81/81 checks and1000/1000 command outcomes, including23 HTTP loopback cases. Host suite now52 passed. Earlier native stale-state failures and USB64-byte fault-ACK delivery are preserved; the ZLP fix passed the final run.

`recovery_20261020T061525Z.json` records11 successful watchdog/NVS/radio checks but the final restored-policy read was interrupted. The runner previously returnedzero despite a restoreexception; now errors causeFAIL/nonzero. Historicalrawdata unchanged. Audioattempts retainUSB/APItimeouts; there is no completed accuracy dataset. The user then reported bothboardsunavailable. Host-onlypreviewtesting is explicitly separate; no hardwarefailure is concealed as a simulatedpass. See the current summary for details.

## Returned B and deep-review implementation

B returned onCOM7. The bounded diagnostic-transfer change fixed the reproduced audio upload timeout. PCM CRC verification was added; the current full corpus completed18 inputs with bit-exact PCM but0/16 command detections. This negative result remains retained at `audio_20261020T081414Z.json`.

The deep-review extension adds a fixed-memory evidence observation monitor,23 cases and a4096-condition host ablation matrix. Current B trace `single_b_20261020T080754Z.json` passed32 API/shadow rows. Host regression83 passed. The monitor is shadow-only, not physical feedback or a complete safety proof. Both native targets compile; onlyB is flashed. New primary prior art narrows the novelty candidate; see the current response/dossier. No historical raw files were modified.
