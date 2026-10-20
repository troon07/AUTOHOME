# ClawGrid current handoff

Updated 20 October 2026. Read AGENTS.md first. Historical claims about a neural text agent, phone STT, working ESP speech, split inference and implicit room inference were incorrect or superseded. Current firmware uses an explicit deterministic parser and confirmed control flow.

For a self-contained new-chat transfer, read [NEXT_CHAT_HANDOFF.md](NEXT_CHAT_HANDOFF.md). For the submitted-versus-executed comparison, read [PROPOSAL_VS_IMPLEMENTATION.md](docs/PROPOSAL_VS_IMPLEMENTATION.md). These include the latest RTOS validation numbers and the remaining proposal gaps.

## Active hardware and permission

- COM5: Node A, ESP32-S3 N16R8, MAC e0:72:a1:f6:a9:68.
- COM7: Node B, ESP32-S3 N16R8, MAC e0:72:a1:f5:bc:b0.
- User confirmed bare boards only and approved identity reads, firmware uploads and memory testing. No relays, loads, microphones or PIR are connected in the current test setup.
- Full pre-upgrade flash backups are in ignored `benchmarks/backups/`.

## Current implementation

A owns the SafetyKernel, bedroom PIR policy, optional motion fan policy, Home/Away/Sleep/All-off scenes, manual holds, relative timers, command replay gate and a 32-entry audit ring. Real GPIO outputs are only pins 4/5, with PIR input 6. Active-low relay polarity remains unverified physically.

B provides a WPA2 AP and HTTP dashboard, token-protected control/transaction endpoints, one pending acknowledged command with bounded retries, strict text proposal confirmation and actual room telemetry with a five-second stale limit.

Radio uses paired encrypted unicast with PMK/LMK, pinned MAC and mandatory application HMAC. Commands bind to the actuator boot nonce and ordered ID. Duplicate requests return cached results; changed same-ID requests are rejected. ACKs echo the exact command payload. Secure boot/flash encryption/HTTPS are not implemented.

Correct N16R8 partitions and explicit PSRAM allocation are configured. Runtime memory measures free/min/largest blocks and task stack margins. The model arena is unallocated normally: no learned model/audio pipeline is deployed. Source files under `firmware/common/runtime/` implement bounds and diagnostics.

Course alignment: read `docs/EMBEDDED_SYSTEM_DESIGN.md`. A now has explicit static ControlTask (core 1, priority 5) and LinkTask (core 0, priority 3), each targeting 5 ms; B has static LinkTask with an independent SDK HTTP task. ControlTask alone owns runtime GPIO state. Static command/ACK queues, a latest-state mailbox, readiness event groups, atomic counters, queued diagnostics and per-task watchdog registration are implemented. Serial `rtos` records and `/api/rtos` expose observed timing/stack data. The build still uses Arduino-ESP32 with ESP-IDF FreeRTOS APIs, not a native `app_main`/CMake project.

## Build and test

```powershell
python scripts/provision_local.py
python -m pytest tests/ -q --junitxml=benchmarks/raw/host_tests.xml
python -m platformio run -e node_voice -t upload --upload-port COM5
python -m platformio run -e node_intelligence -t upload --upload-port COM7
python scripts/research_hil.py --bare-boards --trials 1000 --soak-seconds 180
```

Build sequentially; concurrent PlatformIO runs can collide in shared bookkeeping. Provisioning creates ignored `local_credentials.json` and `firmware/common/config/private_config.h` once and preserves existing credentials. Never print/publish them or a credential-bearing image.

The current host suite has 40 passing tests. Latest measured hardware results, raw capture path and firmware hashes are in `benchmarks/reports/HARDWARE_TEST_RESULTS.md` and its linked JSON. Do not assume all tests pass without reading the current report. Earlier failed runs are deliberately retained.

USB commands: A/B `t` self-test, `m` PSRAM probe; A `a` audit, `d` suppress next ACK, `O` mute application radio frames eight seconds; B `H` HTTP loopback, `S` state, `C op device action value` + newline to queue a control, `F/R/V/W` bad-tag/exact-duplicate/changed-duplicate/wrong-boot injection. Fault commands are for bare-board diagnostics only. The HTTP diagnostic services radio frames during loopback waits to avoid starving the queue.

## Research evidence and next gates

Read `docs/PUBLICATION_TEST_MATRIX.md`, `docs/LITERATURE_REVIEW.md` and `docs/HOME_AUTOMATION_FEATURES.md`. The current research suite measures software state, radio acknowledgement and runtime memory on bare boards. Microphone speech, learned occupancy prediction, single/dual model comparison, electrical load behavior, energy, multi-day occupancy utility and long-duration operation remain pending.

The 2026 paper *Split CNN Inference on Networked Microcontrollers* is direct prior art; two MCU cooperation is not itself a novel contribution. Keep all data tagged SIMULATED / HOST_MEASURED / HARDWARE_MEASURED. Browser speech is not offline board speech, software on-time is not electrical energy, and a PIR silence interval is not an occupancy label.
