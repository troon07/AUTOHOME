# Embedded system design aligned with the submitted ClawGrid proposal

The submitted report is titled *ClawGrid: A Switchless Swarm Home Automation System*. Its software table explicitly calls for ESP-IDF + FreeRTOS, task isolation, watchdogs and OTA. Its architecture also calls for room autonomy, sensor interfaces, deterministic control, communication, physical verification and power management. Edge AI is one subsystem within that embedded controller.

This review uses the submitted text and its embedded logical architecture figure. The source DOCX is unchanged. The report is a project proposal, not evidence that all listed hardware or software exists.

## How the two-board prototype maps to the submission

| Submitted element | Current two-board role | Scope and correction |
|---|---|---|
| Room controller | Node A: bedroom policy, SafetyKernel, output drivers | One assigned room; local behavior independent of the gateway link |
| Main gateway | Node B: browser API, authenticated command serialization, telemetry | Separate MCU; no shared RAM with A |
| ESP-IDF + FreeRTOS | Explicit static FreeRTOS application tasks using the Arduino-ESP32 SDK | Uses ESP-IDF RTOS APIs, but the build remains Arduino-framework; it is not a pure ESP-IDF/CMake migration |
| Offline room speech | Future AFE/WakeNet/MultiNet/audio task on A | No microphone capture or deployed speech model yet |
| ESP-Claw / Raspberry Pi NLU | Optional future gateway extension | Not required for the two-board prototype; no large-model runtime deployed |
| PIR + mmWave + door fusion | Current intended sensor input is one bedroom PIR | Bare-board tests have no sensors attached; do not claim sensor fusion or multi-room localization |
| Three-way actuator verification | Current ACK reports the SafetyKernel's software output state | Auxiliary-contact and voltage/current feedback require hardware and drivers; ACK is not physical verification |
| Cooperative load shedding | Research extension after measured load power and priorities exist | No power meter or negotiation implementation; scenes/max-run timers do not establish energy management |
| Battery-backed control branch | Hardware extension | No UPS is fitted or power-failure autonomy measured |
| MQTT, SQLite, OTA | Optional management/lifecycle extensions | Not currently implemented; OTA partition reservation alone is not an updater |
| Secure local bus | Paired encrypted unicast, HMAC, ordered requests and cached ACKs | Secure boot/flash encryption/hash-chained persistent audit still pending |
| Native app | Standard local browser dashboard | Consistent with the user's phone constraint; no phone modification |

The submitted figure routes room voice toward the optional edge-AI host, while its text says fixed commands work without that host. The revised architecture should draw the fixed-command path directly from the room microphone to the room controller, and draw complex NLU as a separate optional proposal path. Both paths converge on SafetyKernel.

## Implemented RTOS task allocation

| Board / task | Core | Priority | Release pattern | Stack allocation | Ownership |
|---|---:|---:|---|---:|---|
| A ControlTask | 1 | 5 | `vTaskDelayUntil`, 5 ms target | 6,144 B static internal SRAM | PIR sampling/debounce, safety tick, local policy, timers, ordered command execution; sole runtime GPIO-state owner |
| A LinkTask | 0 | 3 | `vTaskDelayUntil`, 5 ms target | 6,144 B static internal SRAM | Authenticate/decode RX, queue proposals, send ACKs and periodic full state |
| A diagnostics / Arduino loopTask | SDK default core 1 | SDK default priority 1 | Low-priority polling | SDK-managed | USB console, memory probes, audit export and queued diagnostic printing |
| B LinkTask | 0 | 3 | `vTaskDelayUntil`, 5 ms target | 6,144 B static internal SRAM | Receive authenticated state/ACK, bounded retry and transaction progression |
| B HTTP server | 1 | SDK HTTP default | Request driven, two client sockets | 6,144 B SDK-managed | Bounded HTTP parsing, proposal submission and state/memory/RTOS snapshots |
| B diagnostics / Arduino loopTask | SDK default core 1 | SDK default priority 1 | Low-priority polling | SDK-managed | USB diagnostics and HTTP loopback tests |
| Future audio / predictor | To be measured | Below essential control work | Audio DMA notifications / bounded model jobs | Not allocated yet | Optional learned workload; no direct output writes |

ESP-IDF priorities are fixed-priority preemptive priorities; larger numbers are higher. These tasks do not replace Wi-Fi/idle/IPC system tasks. Core 0 still serves the higher-priority Wi-Fi subsystem. Five milliseconds is a design target, not a proven hard real-time deadline. The firmware records execution maxima, release lateness and missed target deadlines.

## Communication and synchronization

The Wi-Fi receive callback copies into the existing eight-frame static RX queue and returns. A LinkTask authenticates and decodes at most four frames per cycle, then places validated proposals into an eight-command static queue. ControlTask handles at most two proposals per cycle and returns execution outcomes through an eight-ACK queue. Queue-full conditions are counted; a dropped proposal can be retried without bypassing the execution gate.

A depth-one overwrite queue carries the latest complete room snapshot from ControlTask to LinkTask. This deliberately replaces old telemetry rather than growing a backlog. A fixed 16-line diagnostic queue moves formatting/output work away from control and radio tasks; dropped diagnostic records are counted. Only the diagnostics task prints those queued records.

Event-group bits publish task readiness and startup faults. Essential tasks register individually with the five-second task watchdog and reset it after each work cycle. Atomic counters cover cross-core statistics. B's existing short critical sections protect copied state/transaction structures. A's short audit critical section protects a bounded copy of command audit records; no network operation or serial printing occurs inside it.

These mechanisms demonstrate real producer/consumer ownership and bounded resource use. Do not add empty tasks, semaphores or interrupts merely to list RTOS features.

## Embedded engineering evidence

The RTOS-extended hardware run on 20 October passed 70 check rows and 1,000
command outcomes. Measured execution maxima were A ControlTask 311 us,
A LinkTask 1206 us and B LinkTask 1031 us, with zero observed misses of the
five-millisecond targets in that short run. Minimum task stack free bytes
were 4140, 4152 and 3600 respectively. These are observations, not WCET proofs.
End-to-end ACK timing was 23.919 ms median and 78.845 ms p95; it must not be
confused with control-task execution time. See the linked raw evidence in the
[RTOS validation summary](../benchmarks/reports/PROJECT_VALIDATION_SUMMARY.md).

| Topic | Current implementation / evidence | Additional course or research demonstration |
|---|---|---|
| Task scheduling | Static tasks, priorities, affinity, periodic release, measured lateness | Trace under concurrent audio/model/HTTP load; compare a single-loop baseline |
| Inter-task communication | Static queues, snapshot overwrite, readiness events | Deliberate queue saturation and bounded recovery |
| Concurrency | One actuator owner; short snapshot locks; atomic counters | Stress shared-state access and document lock budgets |
| Memory | Static internal stacks/queues; flash UI/models partition; explicit PSRAM arena | Peak activation/audio buffers, fragmentation and allocation refusal |
| Drivers and interfaces | GPIO polarity abstraction in SafetyKernel, PIR polling/debounce, USB console, Wi-Fi/ESP-NOW | Actual PIR/driver circuit validation; I2S DMA microphone; optional I2C/UART sensors only when fitted |
| Timing | Per-task jobs/execution/release lateness/missed targets; device-local ACK RTT | Common-timebase acoustic-to-output measurements; IRQ latency if interrupts are introduced |
| Watchdogs and recovery | Essential-task registration and heartbeat are measured | Induced task hang, watchdog reset reason, brownout and GPIO boot waveform |
| Electrical design | Low-voltage pin map and inactive-latch initialization | Driver schematic, protection/flyback, decoupling, current budget and measurements |
| Power | Software on-time estimates and optional-work memory admission | Externally measured board/load energy; backup source only if actually fitted |
| Lifecycle | Reproducible board config and partition layout | Signed OTA, rollback, versioned persistent configuration and power-interruption tests |

The current Arduino layer is a hardware/SDK convenience, not proof that a pure ESP-IDF requirement has been met. If the course requires native `app_main`, ESP-IDF components, Kconfig and CMake, migrate the build as a separately tested step. The task ownership and SafetyKernel boundary should remain the same.

## What to tell the evaluator

Describe ClawGrid as a **FreeRTOS-based distributed embedded home automation controller with an edge-AI extension**. Explain task design, queue ownership, driver circuits, scheduling measurements and recovery before presenting model results. Demonstrate local control during gateway outage, and identify sensor/feedback/power claims that remain hardware work.

The original report's mains contactors, panel protection and UPS describe a larger installation. This project phase remains low-voltage DC only. Software cannot establish electrical isolation, backup duration or continued load power. Acknowledged commands, task watchdogs and physical three-way verification are three different concepts and must have separate evidence.

## References

- Submitted proposal: `C:/Users/sthar/Downloads/FINAL_MITHUN (1).docx` (text and embedded figure reviewed).
- [ESP-IDF 4.4.7 FreeRTOS task and static allocation APIs](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32s3/api-reference/system/freertos.html).
- [ESP-IDF 4.4.7 watchdog APIs](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32s3/api-reference/system/wdts.html).
- [Publication test matrix](PUBLICATION_TEST_MATRIX.md), [current hardware evidence](../benchmarks/reports/HARDWARE_TEST_RESULTS.md).
