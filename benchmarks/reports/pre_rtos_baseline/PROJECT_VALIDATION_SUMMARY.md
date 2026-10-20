# ClawGrid implementation and validation summary

20 October 2026. Both boards have the upgraded controller firmware. The physical setup is USB-powered bare ESP32-S3 N16R8 boards only.

| Item | Result | Source / scope |
|---|---|---|
| Host regression | 40 tests passed | HOST_MEASURED software tests; [JUnit](../raw/host_tests.xml) |
| Hardware check rows | 57 / 57 passed | HARDWARE_MEASURED; includes grouped and individual checks, not 57 independent experiments |
| Sequential idempotent OFF requests | 1,000 / 1,000 expected outcomes | HARDWARE_MEASURED software/radio ACK path; output already OFF |
| Median / p95 / p99 ACK RTT | 11.252 / 15.291 / 25.323 ms | B's device clock; one bench session |
| Mean ACK RTT | 10.337 ms | 95% within-session bootstrap CI 10.118–10.563 ms |
| Completion interval | 99.617–100% | Wilson 95%; zero observed failures does not prove zero failure probability |
| Delayed USB diagnostic retrieval | 5 read-only recovery queries | Count retained; no actuator command re-execution for collection |
| Minimum sampled internal free | A 290,292 B; B 273,976 B | HARDWARE_MEASURED; 64 A / 63 B samples |
| Minimum largest internal block | A 278,516 B; B 262,132 B | HARDWARE_MEASURED |
| RX queue drops / TX enqueue failures | 0 / 0 on both boards | Recorded counters; not RF packet-loss rate |
| Runtime/PSRAM/security/automation checks | Passed | Board boot tests and explicit PSRAM probes |
| Test interval | Approximately 323 s, including a 180 s soak | Short validation only |
| Final software output state | Light OFF, fan OFF, Home; automatic light on, automatic fan off; grace 120 s | Last captured state; settings are volatile |

Firmware build sizes (HOST_MEASURED): A static RAM 46,520 B, application flash 694,317 B; B static RAM 48,152 B, application flash 766,997 B. Each application slot is 3 MiB. Static RAM excludes runtime allocations.

The controls include Home/Away/Sleep/All-off scenes, per-device auto-off timers, manual holds/resume, optional motion fan policy, authenticated command/ACK/retry, stale-state refusal and actual telemetry. See [feature/API guide](../../docs/HOME_AUTOMATION_FEATURES.md).

## Evidence and tables

- [Complete hardware result table](HARDWARE_TEST_RESULTS.md), [CSV](HARDWARE_TEST_RESULTS.csv), [individual trials CSV](COMMAND_RTT_TRIALS.csv).
- [Raw final JSON](../raw/hardware/research_20261020T033845Z.json), [serial log](../raw/hardware/research_20261020T033845Z.log), [statistics JSON](hardware_statistics.json).
- [30-category publication matrix](../../docs/PUBLICATION_TEST_MATRIX.md), [matrix CSV](PUBLICATION_TEST_MATRIX.csv), [literature review](../../docs/LITERATURE_REVIEW.md).
- [Validation history and corrections](../../docs/VALIDATION_HISTORY.md). Failed runs remain in raw evidence.

![Measured command RTT and internal memory](figures/hardware_runtime.png)

Standalone [SVG figure](figures/hardware_runtime.svg) is available for export. The figure shows observations from this run only.

## Remaining publication gates

No learned model or microphone pipeline is deployed. Acoustic accuracy, independently labeled occupancy prediction, single/dual model comparisons, physical low-voltage output behavior, electrical energy, external-browser/load behavior, power-cycle recovery and 24–72 hour concurrent workload tests remain pending. These are not supplied by bare-board diagnostic timings. The literature review includes direct prior work on MCU split inference, so novelty still needs a specific implemented and evaluated contribution.
