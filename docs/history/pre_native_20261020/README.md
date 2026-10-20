# ClawGrid: dual ESP32-S3 home automation research prototype

ClawGrid controls one bedroom's low-voltage light and fan through Node A's deterministic SafetyKernel. Node B provides a protected local browser dashboard and authenticated acknowledged controls. Both boards are N16R8 modules: 16 MiB flash and 8 MiB PSRAM each. Their memories are separate.

This is an **embedded-system design project with an edge-AI extension**. [The submitted-proposal mapping and RTOS architecture](docs/EMBEDDED_SYSTEM_DESIGN.md) cover task scheduling, drivers/interfaces, queues, watchdogs, memory, timing, electrical design and recovery.

[Proposal versus executed work](docs/PROPOSAL_VS_IMPLEMENTATION.md) records completed, partial and pending capabilities. [New-chat handoff](NEXT_CHAT_HANDOFF.md) provides a self-contained continuation summary.

The current bench has **bare boards with no sensors, microphones, relays or loads connected**. Hardware tests verify software state, memory and radio behavior. They do not establish physical-load behavior, neural accuracy or electrical energy savings.

## Implemented

- Light/fan controls; Home, Away, Sleep and All-off scenes.
- Per-device relative auto-off timers, manual holds and resume-auto controls.
- Debounced single-zone PIR lighting policy; optional motion fan policy; configurable vacancy grace.
- Three-second dwell, four-hour continuous-run limit, explicit device whitelist.
- Paired encrypted ESP-NOW unicast with a mandatory HMAC tag, boot-bound command IDs, cached duplicate ACKs and bounded retries.
- Local browser dashboard with actual state, stale-state handling, text proposal confirmation and memory diagnostics.
- Fixed RX queues/application buffers, explicit aligned PSRAM arena, correct 16 MiB partitions, runtime probes and a bounded command audit ring.
- Explicit static FreeRTOS ControlTask/LinkTask allocation, periodic scheduling, core affinity, readiness event groups and task watchdog registration. USB diagnostics run outside the control task.
- Reproducible host/HIL test runner and generated research tables retaining failures and raw traces.

See [full feature/API guide](docs/HOME_AUTOMATION_FEATURES.md) and [architecture](docs/ARCHITECTURE.md).

## Build and use

```powershell
python -m pip install -r requirements-dev.txt
python scripts/provision_local.py
python -m platformio run -e node_voice -t upload --upload-port COM5
python -m platformio run -e node_intelligence -t upload --upload-port COM7
```

Build sequentially. Provisioning generates unique local credentials and an ignored private header; existing credentials are preserved. Join **ClawGrid-Assistant** with the password from `local_credentials.json`, open `http://192.168.4.1`, and enter the local API token. The token remains in the browser tab's memory. See [setup and API](docs/HOME_AUTOMATION_FEATURES.md).

## Reproduce tests and tables

```powershell
python -m pytest tests/ -q --junitxml=benchmarks/raw/host_tests.xml
python scripts/research_hil.py --bare-boards --trials 1000 --soak-seconds 180
```

The HIL runner intentionally changes GPIO state and injects application faults. Use it only with bare boards. It saves JSON/log evidence under `benchmarks/raw/hardware/`, table/CSV results under `benchmarks/reports/`, and exits nonzero on failure. The latest report describes its exact measured scope.

- [Measured hardware test table](benchmarks/reports/HARDWARE_TEST_RESULTS.md)
- [CSV test table](benchmarks/reports/HARDWARE_TEST_RESULTS.csv)
- [Individual command RTT trials](benchmarks/reports/COMMAND_RTT_TRIALS.csv)
- [Publication test matrix and pending experiments](docs/PUBLICATION_TEST_MATRIX.md)
- [Memory/runtime design](docs/MEMORY_AND_RUNTIME.md)
- [Literature and research choices](docs/LITERATURE_REVIEW.md)

The legacy `scripts/benchmark.py` and host simulator produce **SIMULATED** model scenarios, not deployed inference results. They are separate from the real-board HIL suite.

## Research scope

No learned model, audio capture or tensor splitting is deployed. The proposed next step is ESP-SR AFE/WakeNet/MultiNet on A and a small predictor on B, evaluated against simple baselines with independently labeled data. See [model evaluation](docs/MODEL_EVALUATION.md). A research claim needs those measurements and the corresponding gates in the publication matrix.

Low-voltage DC only; no mains switching. A single PIR supports only its assigned zone. The phone remains a standard browser client. Prototype credentials are compiled into firmware; provision each pair separately and do not publish private headers, credentials or credential-bearing binaries.
