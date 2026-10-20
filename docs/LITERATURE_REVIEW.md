# Focused literature review and research positioning

Sources checked 20 October 2026. This is a targeted primary-source review, not an exhaustive systematic review. Findings from other hardware/datasets are not ClawGrid measurements. Where only the abstract or dataset description was inspected, that limit is explicit.

| Work / source | Relevant finding | Consequence for ClawGrid | Review scope |
|---|---|---|---|
| [MCUNet: Tiny Deep Learning on IoT Devices, 2020](https://arxiv.org/abs/2007.10319) | Joint architecture/runtime design addresses MCU memory constraints | Choose operators and schedule activation memory together; raw parameter count is insufficient | Abstract and official project description |
| [MCUNetV2, 2021](https://arxiv.org/abs/2110.15352) | Patch-based scheduling addresses activation-memory bottlenecks | Profile activation peak before assuming another board is needed | Abstract |
| [MicroNets, 2020/2021](https://arxiv.org/abs/2010.11267) | Networks can be designed for commodity MCU constraints | Compare task-specific small models before attempting generative language models | Abstract |
| [MLPerf Tiny Benchmark, 2021](https://arxiv.org/abs/2106.07597) | Benchmarking evaluates inference quality, latency and energy together | Use identical inputs/quality gates and measure electrical energy separately | Abstract; [official benchmark repository](https://github.com/mlcommons/tiny/blob/master/benchmark/README.md) |
| [RockNet: Distributed Learning on Ultra-Low-Power Devices, 2025](https://arxiv.org/abs/2510.13320) | Communication-aware specialized classifiers support distributed learning on a hardware testbed | Two-device cooperation alone is not novel; distinguish training from inference and task allocation | Abstract; numerical claims not transferred |
| [Split CNN Inference on Networked Microcontrollers, IEEE WoWMoM 2026](https://arxiv.org/abs/2605.09357) | Sub-layer partitioning distributes parameters/activations; evaluated across networked MCUs | A generic claim of split inference on microcontrollers already has direct prior art; compare actual split granularity and memory/communication cost | Abstract and bibliographic metadata |
| [ESP-SR official getting started](https://docs.espressif.com/projects/esp-sr/en/latest/esp32s3/getting_started/readme.html) | AFE, WakeNet and MultiNet provide an MCU voice integration path | Start with the supported speech example, pin SDK/model versions, then measure the chosen microphone | Official documentation |
| [ESP-IDF 4.4.7 ESP-NOW](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32s3/api-reference/network/esp_now.html) | Encrypted unicast is supported; MAC delivery does not establish application execution | Paired unicast plus application authentication, ordered IDs, cached ACK and bounded retry | Official security/callback/delivery sections |
| [UCI Room Occupancy Estimation](https://archive.ics.uci.edu/dataset/864/occupancy%2Bdetection) | Four days of controlled room data include manually recorded occupancy counts and multiple environmental sensors | A public baseline can use a single PIR feature, but full-sensor performance does not transfer to this one-PIR prototype | Dataset metadata; data not trained here |
| [Neural Network-based Occupancy Detection on the Edge: dataset/code, 2024](https://zenodo.org/records/11573783) | Motion and image-derived occupancy labels are separate; environmental data span two office rooms | Labels need independent evidence; sensor sampling and office-to-home domain differences matter | Dataset description; files not analyzed |

## What can support a useful paper

The defensible candidate is **deadline- and memory-aware semantic task allocation for local home control under concurrent inference, dashboard traffic and radio loss**. The proposed contribution must be implemented and compared: a rule that admits or defers optional prediction work based on internal-memory headroom and control deadlines, while preserving a measured local safety path. The project now provides the native bounded controller, MultiNet6 model initialization and synthetic-house backend. It does not yet provide the completed learned-workload comparison.

Compare single-board execution, parallel responsibilities, and optional tensor splitting using the same learned model and quality target. Show when communication and the second board's idle power erase the benefit. Include loss/reboot recovery, occupied-time lighting utility and independently measured energy. A careful negative result can be useful if it identifies a reproducible operating boundary.

Novelty is not established by having two ESP32-S3 boards, a browser UI, encrypted commands or a SafetyKernel. The 2025–2026 distributed work makes that distinction especially necessary. A full prior-art comparison should read the complete papers and their released code before a novelty claim is written.

## Model choice

Use ESP-SR as the initial speech candidate. For occupancy, begin with a statistical timer and a compact quantized classifier on causal PIR features; choose a larger temporal model only if held-out utility improves. A learned probability of human absence requires independent occupancy labels. A predictor trained on future PIR events must be described as future-motion prediction.

Browser PCM can supply acoustic inputs to B without a wired board microphone. The incomplete current campaign has no validated acoustic accuracy; real lighting comfort still requires occupancy labels and physical loads. The publication matrix explicitly reserves those claims until the microphone, PIR, low-voltage loads, labels and power measurements are available.

## Deployment update: model choice and version boundary

[Official MultiNet command documentation](https://docs.espressif.com/projects/esp-sr/en/latest/esp32/speech_command_recognition/README.html) distinguishes model generations: MultiNet6 English accepts grapheme command strings directly, while phoneme workflows differ for other versions. It recommends the audio front end with MultiNet. The current baseline deliberately uses push-to-talk raw PCM; that limitation must be evaluated, not hidden.

[ESP-SR v1.7.0 manifest](https://github.com/espressif/esp-sr/blob/v1.7.0/idf_component.yml) and the local pinned component identify the compatible SDK dependency. Pinning an older compatible release is a reproducibility decision, not evidence that it is more accurate than newer models. The exact downloaded-file provenance is in `speech/dependencies_manifest.json`; the current tested model hash is in the validation summary.

For the paper, record full-text screening, inclusion/exclusion reasons, model/task/dataset differences and release versions before claiming a systematic review or novelty. The table above remains a focused review; it does not establish that every relevant publication was read.

## Deep-review follow-up on 20 October 2026

The supplied deep-research DOCX was read separately, with 33 linked entries recovered. Its integration-based novelty argument was narrowed after checking primary runtime-assurance and evidence-freshness work. See [the response](DEEP_REVIEW_RESPONSE.md), [closest-work matrix](NOVELTY_CASE.md) and [portable bibliography](research_references.bib). Direct prior art includes Black-Box Simplex, NASA formal RTA and Mission-Aware Attestation Envelopes (arXiv2610.08771, 6 October2026). No first-ever safety kernel, lease or distributed-MCU claim is made. Some publisher full-text requests were rate-limited; unverified numerical claims remain unverified rather than copied as confirmed facts.
