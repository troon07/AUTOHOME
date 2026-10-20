# Embedded system design and submitted-project alignment

ClawGrid is an embedded home controller with an edge-speech subsystem. The submitted *ClawGrid: A Switchless Swarm Home Automation System* calls for ESP-IDF/FreeRTOS, room autonomy, safety, sensing, communications, recovery and power design. Native ESP-IDF now implements the software foundation; virtual sensors do not fulfil the proposal's physical electronics.

## Native build and startup

A uses `native/`, B uses `speech/`, both native ESP-IDF 4.4.7/CMake with `app_main`. PlatformIO pins the platform/toolchains; the speech component pins ESP-SR 1.7.0 and esp-dsp 1.2.1. CPU is 240 MHz, flash QIO 80 MHz, octal PSRAM 80 MHz, RTOS tick 1 kHz. B uses 64 KiB data cache with 64-byte cache lines for speech. These are configurations, not comparative performance conclusions.

Startup initializes NVS without erasing failures, performs self-tests and enables essential tasks only when readiness/health checks succeed. A commits its persistent boot counter before command execution. A pending OTA image is confirmed only after essential startup health; this is not a signed updater or an OTA campaign.

## Task and ownership table

| Board / task | Core | Priority | Release | Stack | Owned work |
|---|---:|---:|---|---:|---|
| A ControlTask | 1 | 5 | 5 ms periodic, vTaskDelayUntil | 6144 B static internal | SafetyKernel, PIR policy, timers, commands, four virtual rooms; sole runtime device-state owner |
| A LinkTask | 0 | 3 | 5 ms periodic | 6144 B static internal | Authenticate/decode, queue intents, ACK/full state, virtual state |
| A Diagnostics | 1 | 1 | Poll/yield | 8192 B static internal | USB, memory probes, bounded audit export, coalesced NVS commits |
| B LinkTask | 0 | 3 | 5 ms periodic | 6144 B static internal | State/ACK, bounded retry and transaction progression |
| B HTTP | 1 | SDK default | Request driven | 6144 B SDK-managed | Bounded token/API handling; raw PCM upload; no direct GPIO |
| B SpeechTask | 1 | 2 | Notification per bounded audio job | 12288 B static internal | MultiNet6 recognition, clip clearing, proposal/metrics snapshot |
| B Diagnostics | 1 | 1 | Poll/yield | 8192 B static internal | USB HTTP tunnel, self-tests and diagnostic records |

SDK Wi-Fi/idle/IPC/network tasks remain present. A core assignment is not exclusive ownership of that CPU. Higher FreeRTOS priority numbers preempt lower priorities; the speech loop yields per frame. Five milliseconds is a target, not a proved hard deadline. No artificial RTOS tasks or semaphores were added just to list features.

## Communication and synchronization

Wi-Fi callbacks copy bounded frames to an eight-frame static queue and return. LinkTask authenticates/decode work with a per-cycle bound. A uses eight-command/eight-ACK queues, processes at most two commands per control cycle and uses depth-one overwrite mailboxes for snapshots, virtual inputs and delayed policy persistence. Old telemetry is replaced rather than queued indefinitely. A fixed diagnostic queue moves printing out of essential tasks; overflow/drop counters remain observable.

Event groups express readiness/faults. Short critical sections copy state; atomics count shared events. Do not perform network operations/flash writes while holding a spinlock. NVS writes run in Diagnostics after five seconds of quiet edits, with bounded retry cadence; flash cache stalls can still affect other work and require timing analysis.

Normal commands carry a persistent A boot identity, ordered ID, issuance time/TTL and exact-payload ACK. Expired commands do not execute. Identical retries reuse the cached result; changed duplicate payloads cannot replace it. Unknown/timeout outcomes are not success. The encrypted radio link and authenticated application gate do not supply secure boot or a persistent tamper-evident log.

## Memory and drivers

Internal memory retains task stacks, control state, queues and explicit DMA-capable allocations. B places its bounded 128000-byte audio clip and eligible speech allocations in PSRAM. Immutable UI/model assets use flash. The two boards do not share a heap. See [memory accounting](MEMORY_AND_RUNTIME.md) for actual low-water and fragmentation observations.

The native HAL wraps ESP-IDF GPIO/timer/Wi-Fi/socket and USB Serial/JTAG APIs. GPIO outputs are inhibited in current builds; all eight virtual devices use pin -1. The real bedroom PIR has its own polling/debounce state, but nothing is wired. Browser PCM replaces a physical microphone in this demonstration; it does not validate an I2S/DMA driver. The USB polling console is confined to diagnostics; a full-packet ZLP fix removed a measured fault-ACK timing delay. Later audio transport failures still require board debugging.

## Measured scheduling and recovery

The retained 194.6-second native controller session passed 81 check rows, including task progress, watchdog subscription and stack margins. Observed maximum execution: A ControlTask 420 µs; A LinkTask 1511 µs; B LinkTask 1247 µs. Recorded target misses were zero. Minimum free task stacks were 4016/4096/3244 B respectively. These are short-session observations, not WCET guarantees.

A separate real ControlTask stall produced task-watchdog reset reason 6, increased the NVS boot counter, retained Away/grace policy and recovered radio ACKs with OFF software outputs. Eleven checks passed; final restored-policy verification was interrupted. No brownout, physical GPIO waveform or power-loss continuity was tested.

## Course demonstration and remaining engineering

Present task ownership, queues/mailboxes, clock/heap regions, watchdog reset, command freshness, dropout and deterministic safety before speech results. Demonstrate the four virtual rooms with sensor inputs synthesized by the browser and decisions made by A. Report unknown state when boards are absent.

Physical follow-up requires a low-voltage driver schematic, polarity/protection review, current/power budget, boot-waveform measurement, real PIR traces and actuator feedback. Single/dual equal-model experiments, concurrent speech/control stress, 24–72 hour runs, power-cycle tests, signed OTA and measured energy remain separate gates. Mains switching is prohibited in this phase; software All-off is not a hardwired emergency stop.

[Proposal comparison](PROPOSAL_VS_IMPLEMENTATION.md) · [Current evidence](../benchmarks/reports/PROJECT_VALIDATION_SUMMARY.md) · [Publication matrix](PUBLICATION_TEST_MATRIX.md).
