# ClawGrid context for the next chat

Use this as project context and continuation instructions. Status checked 20 October 2026. Read the workspace files before editing; do not assume previous test results describe a later revision.

## User objective and constraints

Continue ClawGrid in `D:\ESD`. This is an Embedded System Design course project first: home lighting/fan control, RTOS scheduling, interfaces/drivers, memory, timing, watchdogs, electrical design and fault recovery. Edge AI and distributed computing are the research extension. The user wants useful automation, responsible memory use on two ESP32-S3 boards, a literature review and reproducible tabulated research tests.

Read `AGENTS.md`. All output changes must pass SafetyKernel. Current phase is LOW-VOLTAGE DC ONLY; no mains switching. One PIR covers only its assigned room. Keep the phone as a standard browser client. Tag observations SIMULATED / HOST_MEASURED / HARDWARE_MEASURED. Never invent AI, timing, memory, energy or physical-feedback results.

## Submitted proposal versus actual scope

Submitted source: `C:\Users\sthar\Downloads\FINAL_MITHUN (1).docx`, titled *ClawGrid: A Switchless Swarm Home Automation System*. Its text and logical diagram were reviewed, and the DOCX was not modified.

It proposes a gateway plus four room controllers, ESP-IDF/FreeRTOS, room-local offline ESP-SR speech, PIR/mmWave/door fusion, optional ESP-Claw on a Pi/PC, negotiated load shedding, isolated power meters, auxiliary-contact plus voltage/current verification, battery-backed control/network, encrypted ESP-NOW, persistent hash-chained audit, MQTT/database services, signed OTA and secure boot/flash encryption.

Current execution is a TWO-BOARD, ONE-BEDROOM prototype, not that complete installation. See `docs/PROPOSAL_VS_IMPLEMENTATION.md` for the detailed completed/partial/pending matrix and `docs/EMBEDDED_SYSTEM_DESIGN.md` for course alignment.

## Hardware and test setup

- Node A / COM5 / MAC e0:72:a1:f6:a9:68: ESP32-S3 N16R8, 16 MiB flash, 8 MiB PSRAM.
- Node B / COM7 / MAC e0:72:a1:f5:bc:b0: same chip/memory configuration.
- Last confirmed setup was BARE BOARDS: no PIR, microphones, relays or loads connected. Do not assume future wiring is unchanged before a test that drives outputs.
- User previously authorized board identity reads, firmware uploads and memory testing in that bare-board setup. Original full-flash backups are retained in ignored `benchmarks/backups/`.

## Implemented and flashed

- Node A static ControlTask: core 1, priority 5, 5 ms target, 6144-byte static internal stack. It alone owns runtime SafetyKernel/GPIO state, PIR sampling/debounce, local policies, timers and ordered command execution.
- Node A static LinkTask: core 0, priority 3, 5 ms target, 6144-byte stack. Authenticate/decode packets, queue commands, transmit ACK/state.
- Node B static LinkTask: same core/priority/period/stack; independent SDK HTTP task on core 1 and low-priority diagnostics.
- Static RX/command/ACK queues, depth-one latest-state mailbox, readiness event groups, atomic counters, short snapshot locks, queued diagnostics and five-second essential-task watchdog registration.
- Bedroom light/fan ON/OFF/toggle, 3 s dwell, 4 h continuous-run cutoff, 5 min manual hold/resume, Home/Away/Sleep/All-off scenes, independent relative auto-off timers, optional motion fan policy, configurable vacancy grace.
- Paired CCMP unicast plus mandatory truncated SHA256-HMAC and pinned peer MAC. Big-endian framing, CRC, boot-bound ordered request IDs, cached exact-duplicate ACKs, changed-payload replay rejection and ACK echo of the exact original command.
- One pending B transaction, four maximum attempts at 300 ms intervals; timeout leaves outcome unknown. Telemetry stale after 5 s becomes unknown and new controls are refused.
- B-hosted protected AP/browser UI, token-protected control/transaction APIs, explicit deterministic text proposal preview/confirmation. No neural NLP model.
- Correct QIO/OPI N16R8 board/partitions; fixed buffers, aligned bounded PSRAM arena, headroom/fragmentation/stack telemetry. Model arena normally unallocated.
- Volatile 32-command audit, source-labeled tests, raw JSON/logs, CSV tables and figures. No persistent policy/history database.

Build framework is STILL Arduino-ESP32/PlatformIO, using real ESP-IDF FreeRTOS APIs. Do not call it a pure ESP-IDF/CMake build. No dummy audio/AI tasks were created.

## Latest evidence

Read `benchmarks/reports/PROJECT_VALIDATION_SUMMARY.md` and `HARDWARE_TEST_RESULTS.md`.

- Host tests: 40 passed.
- RTOS hardware check rows: 70/70 passed.
- 1000/1000 sequential idempotent OFF requests completed with expected outcome; no physical load switching.
- ACK RTT median 23.919 ms, p95 78.845 ms, p99 137.830 ms; one USB diagnostic recovery query.
- A ControlTask max observed execution 311 us, zero observed 5 ms target misses, minimum free stack 4140 B.
- A/B LinkTask max observed execution 1206/1031 us; minimum free stack 4152/3600 B; zero observed target misses.
- Minimum sampled internal free: A 270000 B, B 260580 B; recorded RX drops and TX enqueue failures zero.
- Run lasted about 209 s including a 60 s soak. This is not long-term reliability, WCET proof or watchdog-reset recovery validation.
- Raw trace: `benchmarks/raw/hardware/research_20261020T042308Z.json` and matching `.log`.
- Firmware SHA256 A: `3d83325a6fcf240dc7ce5e4ebe280f8e6062e351d8d1d405a46cf64a48ee4981`.
- Firmware SHA256 B: `c214e7ac28c13aedd8c6b2607ea026b2f5143e7f5aba5e34eb83b7ab2c90e524`.

End-to-end ACK latency was higher in the RTOS run than the earlier single-loop session (old median about 11.25 ms, p95 about 15.29 ms). Do not claim a speedup or infer its cause from separate runs. Profile queueing/radio stages under controlled conditions while preserving control-task ownership. Old reports are retained in `benchmarks/reports/pre_rtos_baseline/`; failed raw runs remain retained too.

## Work not completed

1. Actual microphone/I2S/DMA capture, AFE/WakeNet/MultiNet, speech/audio feedback, learned model deployment or split inference.
2. Connected PIR/actuator tests; mmWave/door drivers or multi-sensor fusion; multi-room deployment.
3. Physical three-way verification: command ACK is only software state, not auxiliary-contact/current/voltage confirmation.
4. Energy meters/Modbus, negotiated priorities/shedding, measured energy savings or jittered load restoration.
5. UPS, split power architecture, physical emergency stop/inhibit, brownout and power-cycle waveforms, complete driver schematic/PCB.
6. Native ESP-IDF build, MQTT/database integration, persistent settings/history, OTA/signing/rollback, VPN access.
7. Secure boot/flash/NVS encryption, production key lifecycle, persistent hash-chained audit. Current keys are compiled into firmware; random 32-bit boot nonce is not guaranteed unique across reboots. No explicit per-command cryptographic TTL.
8. Induced watchdog reset/recovery, worst-case timing proof, 24–72 h concurrent audio/model/HTTP stress, external browser usability/load tests.
9. Independently labeled occupancy/speech data, learned-model utility, fair single/dual model comparisons, power-instrument measurements and final publication findings.

## Important code and files

- `firmware/node_voice/src/main.cpp`, `firmware/node_intelligence/src/main.cpp`.
- `firmware/common/runtime/rtos_runtime.h`, `automation.h`, `secure_link.h`, `control_wire.h`, `memory_runtime.cpp`, `state_wire.h`.
- `firmware/common/safety_kernel/`, `firmware/common/neural_nlp/neural_agent.c` (legacy name, deterministic parser).
- `docs/PUBLICATION_TEST_MATRIX.md` (34 categories), `docs/LITERATURE_REVIEW.md`, `docs/DATA_COLLECTION_PROTOCOL.md`.
- `scripts/research_hil.py`, `capture_hardware.py`, `research_stats.py`, `plot_research.py`, `provision_local.py`.

## Reproduction and secrets

```powershell
python -m pytest tests/ -q --junitxml=benchmarks/raw/host_tests.xml
python scripts/provision_local.py
python -m platformio run -e node_voice -t upload --upload-port COM5
python -m platformio run -e node_intelligence -t upload --upload-port COM7
python scripts/research_hil.py --bare-boards --trials 1000 --soak-seconds 60
```

Build/upload sequentially: simultaneous PlatformIO runs previously collided in shared bookkeeping. The HIL runner drives GPIO and injects application faults; use `--bare-boards` only when that setup is actually true. The current browser is at `http://192.168.4.1` on the protected ClawGrid AP.

Credentials are in ignored `local_credentials.json` and `firmware/common/config/private_config.h`. Do not print/publish them, root `key.pem`/`cert.pem`, backups or credential-bearing firmware images. Provisioning preserves existing credentials. The workspace has no `.git` repository at the time of this handoff.

Recommended continuation: close the real low-voltage sensing/actuation and embedded reliability gaps, integrate actual offline speech, then conduct one defensible learned-model experiment. The literature already includes MCU split inference; two cooperating boards alone are not a novelty claim.
