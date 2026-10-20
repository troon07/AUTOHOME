# ClawGrid proposal versus implementation

Status checked 20 October 2026 against the submitted `FINAL_MITHUN (1).docx`, current firmware, configuration and saved test evidence. The submitted DOCX is unchanged.

## Overall position

The proposal describes a multi-room swarm installation with offline speech, sensor fusion, optional external AI, negotiated energy management, physical switching feedback, backup power and production security. The executed project is a two-board, one-zone FreeRTOS controller with an authenticated local command path, browser interface, deterministic output policy and reproducible bare-board tests.

The embedded control foundation is implemented. The complete proposed AI, sensing, electrical verification and energy system is not implemented. Neither a feature count nor the number of passing tests gives a meaningful percentage completion of that larger proposal.

**Status meanings:** Implemented means present in firmware/software; bench-verified means exercised on real bare boards; partial means only part of the proposed capability exists; pending means no implementation/evidence currently supports the claim. No sensors, microphones, relays or loads were connected during the recorded tests.

## Detailed comparison

| Area | Submitted proposal | Executed work | Status and work remaining |
|---|---|---|---|
| System scale | Main safety gateway plus four room controllers | Two ESP32-S3 N16R8 boards: A bedroom controller; B gateway/dashboard | Reduced prototype scope. Multi-room deployment and scaling untested. |
| Board resources | ESP32-S3 modules | Both boards identified as 16 MiB flash and 8 MiB PSRAM each; correct QIO/OPI configuration and partition map | Bench-verified. Memories remain independent; no shared 16 MiB heap. |
| Embedded RTOS | ESP-IDF + FreeRTOS, task isolation | A static ControlTask and LinkTask; B static LinkTask plus SDK HTTP/diagnostics tasks; priorities, affinity and periodic release | Implemented/bench-verified. Native ESP-IDF/CMake build remains pending if required. |
| Task communication | General task-isolation objective | Static RX/command/ACK queues, latest-state mailbox, readiness events, short locks and atomic counters | Implemented/bench-verified. Broader queue-saturation/concurrency campaigns remain. |
| Deterministic control | Intent → permission/policy → SafetyKernel → driver | Explicit bedroom/device whitelist, dwell, manual hold, continuous-run cutoff; ControlTask owns runtime output state | Implemented. Mock-pin timing tests and bare-board command outcomes verified; real driver/load behavior pending. |
| Light/fan control | Lights/fans, hold/release | ON/OFF/toggle, five-minute manual holds, resume auto, optional motion fan policy | Implemented in firmware. No connected light or fan was tested. |
| Scenes and timers | Away/Sleep and policy actions | Home/Away/Sleep/All-off; independent relative auto-off timers and configurable vacancy grace | Implemented/bench-verified. Scenes can partially apply on safety denial; settings/timers are volatile. |
| Local autonomy | Fixed voice/control works without gateway | A's PIR policy, safety tick and timers run independently of B | Architectural support and application-outage recovery tested. Real PIR operation and voice autonomy pending. |
| Offline speech | I2S microphones, ESP-SR WakeNet/MultiNet | No microphone capture, AFE, wake-word, speech recognizer or audio feedback | Pending. The current text parser is not speech recognition. |
| AI/NLU | Optional ESP-Claw on Raspberry Pi/mini-PC with llama.cpp/Ollama | Bounded deterministic room/device/action text parser; explicit confirmation | Scope changed. ESP-Claw, generative NLU and a trained model are absent. |
| Restricted policies | AI-generated restricted JSON policy, validation before execution | Fixed typed operation/device/action/value command schema | Partial. Strict command validation exists; general AI-policy generation/distribution does not. |
| Presence sensing | PIR + LD2410-class mmWave + doorway transition fusion | One-bedroom PIR polling/debounce and occupancy grace-state logic | Partial software only. No connected PIR test, mmWave/door drivers or sensor fusion. |
| Occupancy estimation | Room-specific fused presence | UNKNOWN/OCCUPIED/LIKELY_OCCUPIED/VACANT policy states | Partial. These are PIR/timer states, not independently validated occupancy estimates. |
| Multi-room state | One controller per room and coordinated behavior | Only bedroom can actuate; other parsed room names do not create registered actuators | Pending multi-room implementation/testing. One PIR cannot localize multiple rooms. |
| Local radio | Encrypted ESP-NOW with framing/counter/TTL | Paired encrypted unicast, explicit big-endian framing, CRC, pinned MAC and mandatory HMAC | Implemented/bench-verified. No explicit cryptographic per-command expiry/TTL field. |
| Command execution outcome | Deterministic assertions with state confirmation | Boot-bound ordered IDs, duplicate suppression, payload-bound ACK, four attempts at 300 ms intervals | Implemented/bench-verified. ACK verifies software execution outcome, not physical electrical behavior. |
| Three-way verification | Commanded state + auxiliary-contact state + voltage/current | SafetyKernel output state and ACK only | Not fulfilled. Auxiliary-contact and voltage/current interfaces, consistency checks and fault policies are needed. |
| Energy metering | Whole-house and branch isolated meters/Modbus | Software output-on-time counters only | Pending. No physical power/energy measurements or metering drivers. |
| Cooperative shedding | Load offers based on deferability/occupancy/holds, negotiated selection | No distributed load-priority negotiation or shedding algorithm | Pending. Scene OFF actions and maximum-run cutoff are not negotiated energy management. |
| Progressive restoration | Jittered load restoration when power stabilizes | No measured-power restoration controller | Pending. Needs valid metering and overload/recovery policies. |
| Power continuity | Split control/load supply, battery-backed control bus and network | USB-powered boards | Pending. No UPS, backup-duration or power-loss continuity evidence. |
| Electrical actuation | Contactors with DC coils, auxiliary contacts, isolation and panel protection | Configured low-voltage GPIO channels; no connected drivers/loads in tests | Major hardware work pending. Current phase prohibits mains switching. |
| Physical emergency control | Hardwired emergency OFF/maintenance inhibit | Software All-off scene, still subject to dwell | Not fulfilled. A software scene is not a hardwired emergency stop. |
| Gateway API | FastAPI or equivalent local API | HTTP API runs directly on B; controls require a token on a protected AP | Implemented. External-browser/load testing is separate from on-board HTTP loopback. |
| App | Flutter/Android application for rooms/scenes/diagnostics | Standard browser UI for the one-zone controller | Deliberate replacement, not native-app completion. No phone modification or installation required. |
| MQTT services | Mosquitto management/events/retained state | Direct local HTTP + ESP-NOW | Not implemented. Current dashboard does not depend on a broker. |
| Database and persistence | SQLite/Supabase registry/log/power history | Compile-time registry and volatile 32-command audit ring | Partial. No database, persistent policy/history or reboot restoration. |
| Traffic hash chaining | Tamper-evident chaining of all inter-node traffic | HMAC authentication and volatile command audit | Not fulfilled. Message authentication is not a persistent hash-chained audit. |
| Device security | Secure boot, flash/NVS encryption, key lifecycle | Per-pair generated credentials, protected AP and API token; keys embedded in firmware | Partial. Secure boot/flash/NVS encryption, secure provisioning and rotation remain pending. |
| OTA/lifecycle | Signed OTA, watchdogs and health | Watchdog registration/health telemetry; two application slots reserved | Partial. No updater, signed OTA, rollback or interrupted-update test. |
| Remote access | Optional VPN-only access | No remote-access/VPN implementation | Pending/optional. Current use is local. |
| Electronics design | KiCad PCB/interface/isolation design | Software pin map and inactive-latch initialization | Partial. Full low-voltage schematic, driver review, PCB/enclosure and measurements pending. |
| Reproducibility | VS Code/CMake/Git and testing tools | Pinned PlatformIO/SDK config, scripts, raw traces, hashes, CSV/Markdown results and figures | Improved evidence infrastructure. Build is Arduino-framework; workspace has no Git repository at audit time. |
| Research AI contribution | Intelligent/cooperative operation | Focused literature review, baseline scenarios and 34-category experiment matrix | Preparation only. No learned model, real split inference, measured AI accuracy/speedup or energy benefit. |

## What has improved or become more concrete

1. **RTOS design is explicit and measured.** The proposal named FreeRTOS; the implementation now defines actual owners, cores, priorities, static stacks, queues and task telemetry. This strengthens implementation evidence, not proof of hard real-time correctness.
2. **Command reliability is specified.** Exact duplicate requests return a cached outcome; altered same-ID requests are rejected; ACKs echo their original command. Timeout reports an unknown outcome. These details make the proposal's safe command boundary concrete.
3. **Resource management is specified.** Correct N16R8 flash/PSRAM setup, static task/queue memory, fixed HTTP buffers, aligned bounded PSRAM allocation, headroom checks and largest-block/stack measurements replace vague capacity assumptions.
4. **Home-control behavior is usable.** Scenes, independent timers, manual holds/resume and stale-state refusal are executable functions with saved tests.
5. **Deployment is simpler for this prototype.** B hosts the UI/API locally without an external server or native phone application. This reduces dependencies; it does not replace complex NLU, long-term storage or full multi-room services.
6. **Research evidence is more honest and reproducible.** Raw failures are retained, simulator outputs are labeled, firmware hashes are saved, and tests/statistical summaries are tabulated. Earlier unsupported AI/performance claims were removed.

Encryption, deterministic safety and offline operation were already proposed; implementing them is fulfillment, not a new invention. The two-board/single-zone scope and browser replacement are design changes, not automatic technical superiority. No performance improvement over the proposal can be calculated because it supplied no measured baseline.

## What has not improved or remains a limitation

- **AI functionality:** no model has been deployed; the neural-named parser source contains deterministic rules.
- **Sensing and physical verification:** the proposal remains ahead of the implementation; sensor fusion and electrical feedback are absent.
- **Energy and failure continuity:** no metering, negotiated shedding, UPS or physical power-failure validation exists.
- **Lifecycle/security:** reserved flash space and authenticated packets do not supply OTA, secure boot, encrypted storage or tamper-evident persistent logs.
- **End-to-end latency:** the RTOS run had higher ACK latency than the earlier single-loop session. No causal attribution is established from those separate runs. Control-task isolation should not be presented as an observed end-to-end speedup.
- **Long-term behavior:** short bare-board runs do not establish 24–72 hour stability, occupied-room utility or production reliability.

## Verified results and limits

| Result | Evidence |
|---|---|
| Host regression | 40 passing software tests, HOST_MEASURED |
| Extended hardware checks | 70/70 passing rows, HARDWARE_MEASURED; grouped and individual checks are not independent experiments |
| Sequential requests | 1,000/1,000 expected outcomes for idempotent OFF commands; no physical load switching |
| ACK RTT | Median 23.919 ms; p95 78.845 ms; p99 137.830 ms |
| A ControlTask | 5 ms target; max observed execution 311 us; zero observed target misses; minimum stack free 4140 B |
| A / B LinkTask | Max observed execution 1206 / 1031 us; zero observed target misses; minimum stack free 4152 / 3600 B |
| Watchdogs | Essential-task registration/heartbeat verified; deliberately induced reset/recovery not tested |
| Runtime memory | Minimum sampled internal free: A 270000 B, B 260580 B; recorded RX drops and TX enqueue failures zero |
| Observation interval | Approximately 209 seconds, including a 60-second soak; one two-board bench setup |

See [full current results](../benchmarks/reports/HARDWARE_TEST_RESULTS.md), [raw final trace](../benchmarks/raw/hardware/research_20261020T042308Z.json), [validation history](VALIDATION_HISTORY.md) and [earlier baseline](../benchmarks/reports/pre_rtos_baseline/PROJECT_VALIDATION_SUMMARY.md). A lost-ACK/application-outage test is not a physical power-loss test. Zero observed deadline misses is not a WCET proof.

## Recommended work order

1. **Confirm the course build requirement.** Native ESP-IDF/CMake migration is a separate task if the evaluator requires it; do not remove the explicit RTOS ownership design.
2. **Complete a real low-voltage demonstrator.** Verify PIR and actuator driver/polarity, then record boot/output behavior and add feedback appropriate to the prototype. Develop the schematic, power/current budget and driver protections.
3. **Add actual offline speech.** Connect a compatible microphone and implement I2S/DMA → ESP-SR AFE/WakeNet/MultiNet → validated proposal → SafetyKernel. Evaluate held-out speakers and noise.
4. **Close embedded reliability gaps.** Controlled profiling of the higher ACK tail, queue/CPU stress, induced watchdog recovery, power/brownout behavior, persistent configuration and OTA/rollback.
5. **Choose an energy scope.** If cooperative shedding remains an objective, fit metering and define priorities/holds/restoration before claiming energy management. Backup supply validation is a separate hardware gate.
6. **Complete one defensible research experiment.** Deploy a small learned model using independently labeled data; compare fixed/statistical baselines and single/dual allocation on equal workloads. Measure accuracy, timing, peak memory and electrical energy. Generic two-MCU cooperation alone is not novel.

The next-chat context is saved in [NEXT_CHAT_HANDOFF.md](../NEXT_CHAT_HANDOFF.md).
