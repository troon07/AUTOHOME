> Historical audit of 19 October. Current native ESP-IDF, NVS/TTL, virtual-house and MultiNet6 work supersedes the implementation status below. See [current comparison](PROPOSAL_VS_IMPLEMENTATION.md), [model evaluation](MODEL_EVALUATION.md) and [research literature](LITERATURE_REVIEW.md). Do not cite the old role allocation or pending-feature statements as the present system.

# ClawGrid review and research direction (19 October 2026)

**20 October update:** The critique below records the original audit, not the current implementation. The runtime upgrade replaces native-struct serialization with explicit big-endian bytes and golden tests, adds local vacancy timing, strict explicit-slot text parsing, actual telemetry with stale-state handling, bounded queues and PSRAM allocation, and correct N16R8 partitions. The embedded dashboard now uses text only. See [memory/runtime design](MEMORY_AND_RUNTIME.md) and [current hardware status](HARDWARE_STATUS.md). Speech models, authenticated commands and split inference remain pending.

**Home-control update:** Authenticated encrypted unicast, mandatory HMAC tags, ordered boot-bound commands, cached execution ACKs/retries, scenes, timers and optional fan automation are now implemented. Read [current features](HOME_AUTOMATION_FEATURES.md), [new literature review](LITERATURE_REVIEW.md) and [publication matrix](PUBLICATION_TEST_MATRIX.md) for current status. The tables below remain a historical audit of the original files.

## Verdict

Build a reliable **single-zone, low-voltage lighting/fan controller** first. Run
offline wake-word and command recognition on Node A, and use Node B for a small
occupancy/vacancy prediction experiment and local monitoring. The best supported
voice model family for this hardware is **ESP-SR WakeNet + MultiNet**, with the
Audio Front End (AFE). It is an existing ESP32-S3 offline command recognizer,
not an open-ended language model. Select its exact version after measuring RAM,
flash, latency, and command accuracy on this board and microphone. [1][2][3]

A quantized temporal classifier (e.g., a small 1-D CNN/TCN or GRU) on recent
**bedroom** PIR events is a defensible research candidate. It may propose a
vacancy grace period or probability of *future motion*. It must never directly
change GPIO, claim to localize another room, or treat no motion as proof of
vacancy. Compare it with a fixed 120-second timer and a simple statistical
baseline. Prefer the smallest model that demonstrably improves the application
metric. ESP-DL is a possible deployment path, but its model conversion and
supported operators must be checked before choosing an architecture. [4]

## Repository critique: observed code vs stated system

| Priority | Evidence | Consequence and required change |
|---|---|---|
| Critical | `firmware/node_voice/src/main.cpp` previously turned every CRC-valid `DEVICE_COMMAND` into a bedroom-light **TOGGLE**, regardless of payload. ESP-NOW uses an unencrypted broadcast peer and no source/sequence authorization. | A remote packet could cause an unintended physical change. This handler now rejects all remote commands. Restore remote actuation only after unicast peer identity, authenticated transport, replay rejection, strict command parsing, and an ACK carrying actual SafetyKernel outcome. CRC is error detection, not authentication. [5] |
| Critical | `firmware/node_intelligence/src/main.cpp` previously changed displayed device state before Node A accepted a command, and did not wait for a SafetyKernel ACK. | Dashboard could report success while the physical device stayed unchanged. It now displays parser output as a proposal and does not send a device command. Implement ACK-driven device state before enabling remote control. |
| High | Node A has PIR input and relay control but no I2S audio pipeline, AFE, WakeNet, or MultiNet. `firmware/common/neural_nlp/neural_agent.c` is substring rules, not a trained neural model. | “Offline voice on the boards” is still a planned feature. Integrate ESP-SR using an actual microphone and its ESP-IDF example, then measure it. [1][2] |
| High | The browser dashboards use Web Speech `SpeechRecognition`. Browser implementations may use a server; the firmware UI even advises enabling mobile data after a network error. | Browser speech cannot substantiate the offline claim. Use the on-board microphone for the offline experiment, and keep browser text entry as a separate test interface. [6] |
| High | `simulator/cooperative_model.py` uses hard-coded CPU cycles, 1.2 Mbps transport, 18.5 ms voice time, 4.2 ms state time, and 2.8 ms/chunk. Mode C performs no tensor inference. `scripts/benchmark.py` calls this formula. | Existing numbers are **SIMULATED assumptions**, not empirical speedup or a research finding. Rename the report accordingly and collect raw device traces before publication. |
| High | `simulator/protocol.py` packs network byte order; C firmware serializes a packed native-endian struct. Tests only round-trip Python. | Cross-language packets may fail despite passing unit tests. Define every wire byte explicitly and add Python-to-C golden vectors and two-board tests. |
| High | `docs/REQUIREMENTS.md` calls wake-word, ACK/retry, fallback, and split inference “Implemented.” Their firmware paths are absent or incomplete. | Use a traceability table with `planned`, `simulated`, `bench-verified`, and `hardware-verified`, with test evidence for each. |
| Medium | Node B defaults to bedroom as active room and the parsers often fill an omitted room from context/history. The single PIR covers only bedroom. | Require an explicit room or a fresh, unique evidence source for ambiguous commands. Keep other rooms `UNKNOWN` unless instrumented. |
| Medium | Node A has no vacancy timer that turns its local output off after no motion; Node B's old timeout sent a command that Node A interpreted incorrectly. | Add and bench-test a local vacancy OFF policy through SafetyKernel, independent of Node B. The 4-hour max run is not a useful vacancy policy. |
| Medium | `key.pem` and `cert.pem` are in the project root, and Node B starts an open SoftAP with unauthenticated HTTP routes. | Treat existing TLS key material as exposed until provenance is checked; never use these files for a deployed identity. Provision unique credentials and an authenticated local control path before remote actuation. |

The current 30 Python tests validate host-side simulator behavior. Both
PlatformIO firmware targets compile, but this does not prove ESP-NOW
interoperability, voice recognition, or physical output behavior. The build
logs identify the default `esp32-s3-devkitc-1-N8` board (8 MB flash, no PSRAM)
even though `platformio.ini` overrides selected flash/PSRAM settings for the
claimed N16R8 hardware. Confirm the exact physical module and create a matched
board definition and partition table before loading large speech models.

## Focused literature review

1. **ESP-SR** documents a complete AFE, WakeNet, and MultiNet path on ESP32-S3.
   MultiNet recognizes bounded command phrases offline and expects 16 kHz,
   16-bit mono AFE output after WakeNet. This supports a constrained control
   vocabulary; it does not solve general speech transcription. [1][2][3]
2. **MCUNet** shows that useful neural inference on microcontrollers requires
   model/runtime co-design and careful memory planning. This supports trying a
   compact classifier instead of selecting a model by parameter count alone. [7]
3. **Neurosurgeon** and **Edgent** show that split inference depends on layer
   activation size, local compute, communication, and the partition point. Their
   evaluated systems use mobile/edge or cloud hardware, so their speedups cannot
   be transferred to two ESP32-S3 boards. A measured two-MCU comparison is the
   appropriate gap here. [8][9]
4. Occupancy studies use multiple sensing modalities because presence is not
   identical to PIR motion. With one PIR, prediction is only for its assigned
   zone and needs independent ground-truth labels. [10]

## Publishable question and experiment

**Question:** On two ESP32-S3-N16R8 boards, can uncertainty-aware, bounded
vacancy prediction reduce low-voltage light/fan run time after a room empties
without increasing false-off events for a still occupant, while retaining local
offline operation and deterministic SafetyKernel authority?

The distributed comparison should be a secondary systems contribution:
single-board baseline, task-parallel two-board placement, and split inference
on **the same real quantized model** at several valid cut points. A negative
result for splitting is publishable if the measurements are rigorous. Do not
compare Mode B's command pipeline latency to Mode A's token-generation latency;
those are different tasks.

### Minimum experimental protocol

1. Confirm both board variants, microphone, relay polarity, rail voltage,
   ESP-NOW channel, and real free internal/PSRAM memory at boot. Keep all loads
   low-voltage DC. Use an independent current meter for energy measurements.
2. Record consented, locally stored bedroom PIR timestamps and **independent**
   occupied/vacant labels from an observer or a clearly described reference
   sensor. Split by day/session and participant, never by adjacent time windows.
3. Compare fixed grace, a simple adaptive statistical timer, and one compact
   quantized temporal model. Predefine false-off events per occupied hour,
   excess ON-minutes after vacancy, command accuracy, and energy per hour.
4. Measure p50/p95/p99 latency from event to authorized GPIO transition, peak
   internal SRAM/PSRAM, flash, energy, packet loss, retries, and recovery after
   Node B reboot. Record raw timestamps and firmware/model versions. Label
   results `HARDWARE_MEASURED`; retain simulations as `SIMULATED`.
5. For split inference, transfer actual intermediate tensors and compare
   every partition with the identical model run entirely on Node A and Node B.
   Report bytes, quantization error, ACK/retry cost, and whether memory fit
   changes the result. ESP-NOW MAC success alone does not prove application
   delivery. [5]
6. Show ablations: no model, no Node B, poor link, unoccupied-but-warm room,
   stationary occupant, and browser disconnected. Record every denied intent
   and verify zero unauthorized physical toggles.

### Implementation order

1. Bench-test the existing local PIR-to-SafetyKernel-to-DC-load path, including
   startup polarity, a local vacancy timer, and loss of Node B.
2. Port Node A voice to ESP-IDF/ESP-SR with AFE + WakeNet + MultiNet; retain the
   rules parser only as an explicit text baseline. Measure on the actual board.
3. Make the inter-board protocol interoperable and authenticated; report real
   device state only after an ACK. Bind an intent to room/device/action exactly.
4. Collect ground-truth data; train/evaluate the smallest useful temporal
   predictor offline; deploy it on Node B as a proposal to a bounded timer.
5. Run the matched, hardware-measured single/parallel/split experiments and
   write the paper from the frozen data and failure cases.

## Sources

[1] [Espressif ESP-SR getting started](https://docs.espressif.com/projects/esp-sr/en/latest/esp32s3/getting_started/readme.html)
[2] [Espressif MultiNet command recognition](https://docs.espressif.com/projects/esp-sr/en/latest/esp32s3/speech_command_recognition/README.html)
[3] [Espressif WakeNet](https://docs.espressif.com/projects/esp-sr/en/latest/esp32s3/wake_word_engine/README.html)
[4] [Espressif ESP-DL getting started](https://docs.espressif.com/projects/esp-dl/en/latest/getting_started/readme.html)
[5] [Espressif ESP-NOW guide](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/network/esp_now.html)
[6] [MDN Web Speech API guide](https://developer.mozilla.org/en-US/docs/Web/API/Web_Speech_API/Using_the_Web_Speech_API)
[7] [Lin et al., MCUNet (2020)](https://arxiv.org/abs/2007.10319)
[8] [Kang et al., Neurosurgeon (ASPLOS 2017)](https://doi.org/10.1145/3037697.3037698)
[9] [Li et al., Edgent (2018)](https://arxiv.org/abs/1806.07840)
[10] [Occupancy estimation using audio and sensor modalities (2016)](https://arxiv.org/abs/1602.08507)
