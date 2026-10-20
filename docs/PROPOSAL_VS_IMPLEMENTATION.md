# Submitted proposal versus executed work

Updated 20 October 2026 against `FINAL_MITHUN (1).docx`, source and retained observations. The submitted DOCX is unchanged and is a reference, not permission to activate its proposed mains hardware. User's current scope: finish the virtual demonstration; nothing wired; Node B available on COM7; Node A unavailable.

## Comparison

| Proposed element | Executed work | Status and remaining work |
|---|---|---|
| Gateway + four physical room controllers | Two boards; A controls four virtual rooms, B hosts API/speech | Changed scale. Four physical controllers are not implemented. |
| ESP-IDF + FreeRTOS | Native app_main/CMake; static periodic control/link tasks, diagnostics and speech worker | Implemented; native builds flashed and controller bench-tested. |
| Deterministic safety | Sole A ControlTask owner; every automatic/manual/voice intent goes through SafetyKernel | Implemented; host/MCU software tests. Electrical safety still unvalidated. |
| Room lighting/fans | Physical-bedroom software registry plus eight virtual light/fan devices | Implemented in software; current GPIOs inhibited. |
| Scenes, timers, overrides | Home/Away/Sleep/All-off; relative timers; manual holds/resume; configurable grace | Controller bench-tested. No restored ON output/timer after reboot. |
| Sensor fusion | Separate one-PIR bedroom tracker; virtual room PIR/presence/temperature/lux/door inputs | Synthetic fusion on A implemented; no real mmWave/door/environment driver validation. One PIR cannot localize four rooms. |
| Room localization | Browser person produces explicitly synthetic per-room inputs | Demonstration input generator, not physical positioning. |
| Offline speech | Browser PCM capture; pinned ESP-SR 1.7.0 MultiNet6 English, 16 phrases on B | Model resident/readiness verified; audio transport and human recognition campaign incomplete. |
| AFE/WakeNet/I2S | Push-to-talk raw PCM from browser | Scope changed. No continuous listening, wake word, AFE or board microphone driver. |
| Optional ESP-Claw/Pi generative NLU | Bounded deterministic text preview + constrained learned speech recognizer | No Pi/LLM/free-form assistant. General natural-language reasoning is not implemented. |
| Distributed AI | Speech on B, control on A, compact authenticated messages | Parallel responsibilities implemented. No pooled memory, tensor split or same-model single/dual comparison. |
| Local secure bus | CCMP unicast + pinned peers + mandatory HMAC + boot identity/order + 3 s command TTL | Controller authentication/replay/retry/outage tests pass. Not production security certification. |
| Command execution confirmation | Exact original command echoed in ACK; cached duplicate outcome; bounded retries | Bench-tested; ACK verifies software state, not relay electrical state. |
| Persistent configuration | Versioned explicit-byte CRC record, NVS boot counter, delayed/coalesced policy commit | Saved policy survived real watchdog reset; final original-policy verification interrupted. |
| Three-way physical verification | Software kernel output + ACK only | Not fulfilled: needs auxiliary contact and voltage/current feedback hardware. |
| Load metering/Modbus | Software on-time counters | Physical power/energy sensing absent; on-time is not kWh. |
| Negotiated load shedding/restoration | Scenes, timers and maximum-run cutoff only | No load-offer protocol, measured-power budget or restoration controller. |
| Backup control bus/UPS | USB-powered boards | No backup supply or power-loss autonomy evidence. |
| Contactors/protection/emergency stop | GPIO output inhibition; virtual devices have no GPIO | No completed driver circuit/PCB/electrical tests. Mains prohibited. Software All-off is not hardwired emergency control. |
| Android/Flutter UI | Browser house layout, person, room controls, raw/summary exports and diagnostics | Deliberate browser replacement. Offline layout inspected; full live browser/phone trial pending. |
| MQTT/SQLite/Supabase | Direct on-board HTTP and ESP-NOW; NVS policy; volatile 32-command audit | Broker/database/history service not implemented. |
| Tamper-evident audit | HMAC packets and volatile bounded command ring | No persistent hash-chained traffic log. |
| Secure boot/encrypted flash/NVS | Pair-specific private credentials; no eFuse security activation | Production provisioning, encryption, key rotation and rollback threat model remain. |
| OTA/rollback | Dual app slots; native pending-image startup health confirmation | No signed OTA transport/updater or interrupted-update test. |
| RTOS reliability | Static queues/stacks, short locks, watchdog registration, progress/stack/drop metrics | 81 controller check rows passed; actual watchdog reset observed. Long-duration/audio concurrency and power tests remain. |
| Reproducible research | Pinned dependencies/model provenance, host C/C++ tests, raw failures, hashes, CSV/statistics/figures | Stronger than an unmeasured proposal; publication comparison and independent data still required. |

## Improvements that are now concrete

1. Native ESP-IDF now meets the submitted software-framework requirement, with real ownership, priorities, static queues and measured execution. RTOS is a central part of the system.
2. The four-room browser environment makes automation inputs, dropout and device requests demonstrable without claiming unavailable physical sensors. MCU processing remains required for outputs.
3. A real pretrained speech model is deployed; the deterministic text parser is no longer the only assistant component. Its usability/accuracy is still unproven in the incomplete audio campaign.
4. Policy persistence, persistent boot identity, authenticated command expiry and cached exact-payload ACKs close concrete reliability gaps in the earlier prototype.
5. Memory placement, allocation admission, task stack observations and retained failure logs make resource and recovery claims reviewable.

These are implementation improvements. The submission supplied no measured baseline, so no numerical performance improvement over the proposal can be calculated. The current command measurements also do not establish a causal speedup over earlier Arduino sessions.

## What is actually verified

83 host tests pass. The retained native run has 81/81 passing controller rows, 23/23 HTTP loopback cases and 1,000/1,000 idempotent OFF outcomes. ACK median/p95/p99 are 18.789/44.033/56.754 ms. The 194.6-second session includes a 60-second soak; it is not a long-term reliability test. Model readiness passed; the run did not execute neural audio inference during command timing.

A later watchdog test records 11 passing checks, including reset cause, boot increment, NVS retention and radio recovery. A final policy read was interrupted; the entire campaign is not marked complete. Audio attempts retained transport failures and produced no completed accuracy dataset.

[Full results](../benchmarks/reports/PROJECT_VALIDATION_SUMMARY.md) · [New-chat context](../NEXT_CHAT_HANDOFF.md).

## Remaining work, in order

| Gate | Can finish without boards? | Next action |
|---|---|---|
| Software/docs/interface preview | Yes; completed for current sources | 83 host tests, current tables, offline movement/tour/dropout UI and handoff |
| Live virtual-house demo | No | Return both boards; stream inputs, move through all rooms, verify actual A outputs and browser exports |
| Speech pipeline completion | No | Diagnose USB/API audio timeout, retain mismatches, demonstrate PCM → B model → confirmation → A SafetyKernel |
| Reboot final verification | No | Re-run recovery script and confirm original saved policy after transport settles |
| Acoustic/model evidence | No | Independently labeled held-out humans, negatives/noise/distances, model comparison, peak/concurrent timing |
| Physical demonstrator | No; outside current virtual scope | Review low-voltage PIR/relay wiring, drivers and feedback before enabling any pin |
| Publication claim | Partly planning only | Single/dual equal-workload baselines, long runs, independent energy measurements and full prior-art comparison |

All tasks that require absent hardware remain explicit; layout preview and fabricated inputs cannot complete those measurements.
