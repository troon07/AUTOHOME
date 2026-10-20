# Model choice and evaluation

## Current selection

Node B runs **ESP-SR 1.7.0 MultiNet6 English**. This is a supported, constrained-command baseline for ESP32-S3 and the pinned ESP-IDF 4.4.7 build, not the proven best model. Upstream files are pinned to commit `07b0a4d0f6b5de755874d5405150ee0a8eec9865`; the downloader verifies Git blob identities and records SHA256 provenance. The deployed packed model is 3,788,231 bytes in a 6 MiB flash partition.

Model initialization/readiness passed on hardware. Synthetic audio and human microphone campaigns are incomplete; no recognition accuracy or throughput claim is justified. The deterministic text parser and occupancy thresholds are separate nonlearned components. No learned occupancy predictor, LLM, neural training or tensor splitting is deployed.

## Why this allocation

A owns time-sensitive control and safety; B owns speech, network API and user proposals. Task allocation avoids transferring intermediate neural activations and preserves control ownership. That rationale is not measured superiority. A fair comparison must execute the same speech model, commands and load on one board and two boards, with both boards' power included.

Each board has 8 MiB PSRAM. Flash, internal SRAM and PSRAM have different capacity/access constraints. B retains about 4.09 MB sampled free PSRAM with its initialized model, but internal low-water reached 55,459 bytes in the retained controller session. Spare capacity is for measured headroom, not a reason to fill all RAM. The legacy optional 2 MiB generic arena is unused by ESP-SR; ESP-SR owns its own tensors/allocations.

## Required comparisons

| Question | Experiment | Current status |
|---|---|---|
| Does recognition function? | Fixed TTS smoke corpus plus silence/noise, retained raw inputs/hashes/results | Script/data prepared; hardware run incomplete |
| Does it work for users? | Held-out human speakers/accents; command confusion, false accepts/rejects, noise/distance; clear consent and labels | Not measured |
| Which speech model is best here? | Pin compatible candidates; compare equal phrase inventory, quality gate, latency, resident/peak memory and measured J/command | Not established |
| Does task allocation help? | Single/dual same workload; matched inputs and session-level replication; concurrent HTTP/radio/control metrics | Not measured |
| Is an occupancy model useful? | Independently labeled days; fixed grace/statistical baseline vs compact quantized classifier; false-off occupied minutes | No predictor or dataset experiment implemented |
| Does splitting help? | Same trained model at several split points; accuracy, activation traffic, fragmentation, retry cost, memory and energy | Not implemented; optional research branch |

For human speech, include unsupported utterances and long negative recordings. Confidence scores are not calibrated correctness probabilities. Report capture, transport, model wall time, radio and physical output timing separately. Current command RTT is not speech inference time. TTS-only match rates, once available, must be labeled synthetic smoke results.

## Publication direction

Study **memory- and deadline-aware allocation of speech versus control under interference**, with a defined admission/defer policy and equal-workload baselines. The current architecture is the experimental platform. The causal scheduling experiment, independent quality evaluation, electrical energy measurements and novelty argument remain future evidence. Read [literature](LITERATURE_REVIEW.md) and [test matrix](PUBLICATION_TEST_MATRIX.md); two MCUs alone are not a novel contribution.
