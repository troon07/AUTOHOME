# Embedded controller RTOS validation summary

20 October 2026. Both boards run the explicit FreeRTOS application task architecture. This is a bare-board software/radio test, with no sensors, microphone, relays or loads connected.

| Item | Result | Evidence scope |
|---|---|---|
| Host regression | 40 passed | HOST_MEASURED |
| Extended hardware check rows | 70/70 passed | HARDWARE_MEASURED; grouped and individual checks |
| Command outcomes | 1000/1000 expected ACKs | Sequential idempotent OFF requests |
| Median / p95 / p99 ACK RTT | 23.919 / 78.845 / 137.830 ms | One bench session; not model latency |
| Mean ACK RTT | 34.376 ms | 95% within-session bootstrap CI 33.003–35.903 ms |
| USB diagnostic recovery queries | 1 | Read-only; no command re-execution |

## Task observations

| Board / task | Core | Priority | Period ms | Max execution us | Deadline misses | Minimum stack free B |
|---|---:|---:|---:|---:|---:|---:|
| COM5 / ControlTask | 1 | 5 | 5 | 311 | 0 | 4140 |
| COM5 / LinkTask | 0 | 3 | 5 | 1206 | 0 | 4152 |
| COM7 / LinkTask | 0 | 3 | 5 | 1031 | 0 | 3600 |

Essential tasks registered with the task watchdog. Induced watchdog reset/recovery and a proven WCET bound remain separate gates. Timing uses an ideal five-millisecond progression from each task's initial microsecond phase; tick quantization and first-release phase limit interpretation of release lateness.

## Evidence and course alignment

- [Embedded-system design and submitted-proposal mapping](../../docs/EMBEDDED_SYSTEM_DESIGN.md).
- [Full hardware table](HARDWARE_TEST_RESULTS.md), [test CSV](HARDWARE_TEST_RESULTS.csv), [trial CSV](COMMAND_RTT_TRIALS.csv).
- [Raw JSON](../raw/hardware/research_20261020T042308Z.json) and [serial log](../raw/hardware/research_20261020T042308Z.log).
- [34-category publication matrix](../../docs/PUBLICATION_TEST_MATRIX.md).
- [Earlier controller baseline](pre_rtos_baseline/PROJECT_VALIDATION_SUMMARY.md). Separate firmware/session results must not be pooled as one experiment.

![Current hardware timing and memory](figures/hardware_runtime.png)

Current end-to-end ACK timings are higher than the earlier single-loop bench run. No causal speedup or slowdown attribution is established from these separate sessions. The RTOS change establishes task ownership, scheduling and isolation; controlled profiling of queueing and radio delivery is the next performance step.

The build remains Arduino-ESP32 with ESP-IDF FreeRTOS APIs, not native ESP-IDF/CMake. GPIO/output feedback, I2S/DMA speech, additional sensors, UPS/energy management, OTA, secure boot and learned-model evaluation require further implementation or hardware. No mains switching is permitted.
