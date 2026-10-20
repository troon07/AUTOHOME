# Model selection and evaluation

Updated 2026-10-20. No neural model is currently deployed. The fixed text parser has no learned parameters. Previous speculative token-rate and memory tables have been removed because they were not measurements.

## Recommended candidates

1. **Node A: ESP-SR AFE + WakeNet + MultiNet.** This is the first candidate for offline wake-word and constrained home commands. Select the exact model version using microphone accuracy, latency and measured peak memory; it is not yet established as best by this project's experiments.
2. **Node B: a small quantized temporal classifier**, compared with a fixed vacancy timer and a statistical baseline. Train on independently labeled occupancy data if predicting vacancy, or explicitly predict future motion if only PIR event labels exist. No-motion alone is not a vacancy label.
3. **Split inference: optional controlled experiment.** Run an identical trained model on one board, with parallel responsibilities, and with a selected partition. Account for activation bytes, transport, reassembly, queueing, accuracy, energy and failures. Two boards do not provide one shared RAM pool.

## Memory feasibility gate

For each board, measure weights resident in RAM, activation peak, operator scratch, audio history, task stacks and network/HTTP overhead. Packed weights alone require approximately parameters times bits per weight divided by eight bytes, before metadata and runtime buffers. Flash capacity is not runtime RAM capacity. Record largest free blocks as well as total free bytes.

Use the bounded PSRAM arena and startup headroom checks documented in [memory/runtime design](MEMORY_AND_RUNTIME.md). Its current 2 MiB cap is an engineering guardrail, not a measured optimum. Unused PSRAM is available for future workloads; it is not evidence that a given model fits.

## Acceptance experiment

- Speech: held-out speakers and noise conditions, false accepts/rejects, command accuracy, end-to-end p50/p95/p99 latency, peak memory and energy per command.
- Occupancy: held-out days/households where possible, false-off events, occupied minutes without lighting, unnecessary on-time and calibration. Avoid adjacent-window leakage between training and evaluation.
- Distributed execution: identical inputs and model, per-stage timing, loss/retry counts, activation traffic and single-board baselines. Report negative results if communication costs exceed compute savings.
- Runtime: sustained concurrent audio, inference, HTTP and radio operation, fragmentation, stack margins, resets and overload rejection.

Every benchmark must retain raw traces, firmware/model hashes, configuration and source labels. Bare-board allocator timings are not inference timings. See [literature and research plan](RESEARCH_DIRECTION.md) for primary references and a falsifiable research direction.
