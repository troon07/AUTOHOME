# Returned pair validation 20 October 2026

Both boards are connected through native USB: A COM5 and B COM7. Current firmware was deployed with identity verification and NVS preserved. Physical outputs remain inhibited. Live demo: http://127.0.0.1:8766/simulation (serial bridge session 54826).

| Check | Source | Result |
|---|---|---|
| Project host suite | HOST_MEASURED | 83 passed |
| Paired controller | HARDWARE_MEASURED | 81/81 check rows passed |
| Sequential idempotent OFF commands | HARDWARE_MEASURED | 1000/1000 outcomes passed |
| ACK median / p95 / p99 | HARDWARE_MEASURED | 17.812 / 23.647 / 27.813 ms |
| Watchdog and original policy recovery | HARDWARE_MEASURED | 12/12 passed including final restoration |
| B API and shadow evidence monitor | HARDWARE_MEASURED with SIMULATED monitor inputs | 32/32 passed in sequential rerun |
| Speech command smoke test | SIMULATED PCM / HARDWARE_MEASURED processing | 0/16 commands detected; silence/noise returned no command |

Raw traces: `../raw/hardware/research_20261020T161355Z.json`, `../raw/hardware/recovery_20261020T161424Z.json`, `../raw/hardware/audio_20261020T161700Z.json`, `../raw/hardware/single_b_20261020T161712Z.json`. Host trace: `../raw/host_project_tests_returned_pair_20261020.xml`.

Failures are retained. An initial root-level pytest invocation collected a legacy simulator script requiring a separate unavailable server; the maintained `tests` suite passed. An API campaign overlapping active speech jobs failed its unchanged-job assertion; the sequential rerun passed. That audio run therefore includes concurrent API diagnostics and is not a controlled inference latency baseline.

These tests establish software state, communication, bounded runtime observations, and watchdog recovery. No physical relay, sensor accuracy, energy savings, long-term reliability, or useful speech recognition is established. Prior reports and the Word audit predate this returned-pair campaign; use this dated addendum for the latest deployment and recovery status.
