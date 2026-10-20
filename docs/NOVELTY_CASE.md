# Research contribution and novelty assessment

## Decision

Use **Measuring Fault Observability and Evidence Freshness in a Dual MCU Home Controller** as a working study title. The defensible candidate is an empirical embedded-systems contribution: a reproducible fault benchmark, executable observation contracts and measured resource/availability tradeoffs under concurrent AI and radio work. Novelty is **not established yet**. The current evidence is a host fault matrix and a single-MCU shadow implementation, plus historical two-board controller tests.

A new name for a standard safety state machine does not create novelty. Adding more relays, an LLM, MQTT, Thread or hash chaining also does not establish a contribution. The submitted review's integration argument is a hypothesis to investigate, not evidence that the combination is first.

## Closest prior work

| Primary source | Already established | Consequence for our claim |
|---|---|---|
| [Black-Box Simplex, 2021 preprint](https://arxiv.org/abs/2102.12981) | Runtime checks mediate advanced controllers and baseline recovery | Do not claim invention of AI-proposes/safety-decides |
| [NASA formal RTA framework, 2024](https://ntrs.nasa.gov/citations/20240006522) | Formal construction of runtime-assurance instances | Finite MCU tests are not an equivalent physical safety proof |
| [Runtime verification of sensing/actuating tasks, 2020](https://www.mdpi.com/1424-8220/20/4/1207) | IoT task execution monitoring and verification | An actuator-observation monitor alone is not new |
| [Mission-Aware Attestation Envelopes, 6 October 2026](https://arxiv.org/abs/2610.08771) | Freshness, integrity and decision deadlines in HIL assurance | Evidence age plus a deadline is not a first-ever contribution |
| [Split CNN Inference on Networked Microcontrollers, WoWMoM 2026](https://arxiv.org/abs/2605.09357) | Fine-grained distributed neural execution on an MCU testbed | Two boards/split tensors alone do not establish novelty |
| [MLPerf Tiny](https://github.com/mlcommons/tiny) | Comparable embedded ML quality, latency and energy evaluation | Keep equal workloads/quality and expose power boundaries |

Review scope: primary abstracts and accessible technical sections, not complete replication of each work. Complete full-text/code screening and forward/backward citation search before writing an exclusivity claim. Additional actuator trust-lease work surfaced in secondary metadata but is not promoted as a verified primary-source finding until its publisher/full text is checked.

## Candidate contribution that remains worth testing

Existing research already treats safety, freshness, constrained inference and feedback separately and in broader architectures. Our candidate study asks what can actually be observed and guaranteed from a small, asynchronous home-controller evidence path with two MCU memory budgets. It must quantify when rejecting stale/correlated evidence reduces false verification, when that policy unnecessarily withholds confirmation, and which physical failures remain indistinguishable even after every observable check passes.

The proposed artifact has three parts: a clock/identity/lease-aware observation contract with fixed memory; a joint sensor/network/model-workload fault corpus with explicit hidden simulated truth; and a matched single/dual allocation experiment reporting safety-contract violations, healthy availability, timing, resource peaks and electrical energy together. None is claimed as a first without the closest-work comparison. If the algorithm is not new, a careful empirical benchmark or negative result may be the appropriate contribution and venue.

## What has been implemented

- The actual C++ monitor and 23 host/MCU cases, including boot/command binding, duplicate resistance, clock wrap, fault latch and explicit recovery.
- A 4096-condition Cartesian fault matrix with raw rows and weak ablation baselines. No random dataset split or statistical significance is claimed for this finite constructed set.
- 83 passing host tests; 32/32 B-only API/shadow rows passed. B measured one 23-case self-test at 375 us and object layout at 60 bytes. It did not switch or verify a physical load.
- A checked energy integrator ready for externally captured traces; no electrical energy measurement exists yet.
- Speech transport repaired and PCM checksummed on B. Recognition currently misses the synthetic command corpus, so useful voice and cross-layer inference/control claims remain blocked by a functional issue.

## Falsifiable hypotheses and experiments

| Question | Required experiment | Primary outputs |
|---|---|---|
| Does the contract improve correctness without refusing everything? | Same fault traces with ACK-only, feedback-only, freshness-only and full contract; also a strong existing monitor baseline | Contract violations, false physical assertions, healthy verification availability, refusal reason |
| What is the freshness/availability frontier? | Sweep lease, settle, sample cadence and correlated delay/dropout on held-out traces | Risk/coverage curve and blind-interval duration; retain conservative refusals |
| Does the second board help? | Identical model, audio, UI and control workload on single vs dual allocation; matched quality targets | Command/tick/inference p50/p95/p99, misses, completion, memory peaks, total two-board energy |
| Which faults are not observable? | Independent and common-mode false feedback, stale replay, wrong epoch, sensor loss, stuck actuator, power loss | Counterexample traces and explicit equivalence classes, not unconditional safety assertions |
| Is the behavior useful in rooms? | Independent occupancy/actuation ground truth; held-out days and human speakers | False-off occupied time, unnecessary ON time, command confusion, negative false accepts and comfort violations |

The existing weak ablations demonstrate mechanism, not superiority to the state of the art. A monitor that always refuses can have zero false verification; availability and cost must accompany every such result. Current matrix counts must not be presented as real-home failure rates.

## Experimental discipline

Use only low-voltage lights/fans in a later physical phase. Keep policy/model outputs advisory. Specify accepted actions and their SafetyKernel outcomes separately from auxiliary/electrical verification. Collect independent truth; do not train an occupancy model on no-motion-as-vacancy labels. Keep TTS smoke clips separate from held-out human evaluation.

Before the main comparison, freeze a workload, model, acceptance bounds, primary metric and analysis plan. Randomize or counterbalance condition order across independent sessions. Use session/day-level or block resampling where serial dependence matters. Report failures, censored/unknown outcomes and all denominator choices. A 1000-command run from one boot is not 1000 independent deployments.

Use raw externally measured V(t), I(t) with common-timebase event markers. Include failed attempts, idle/listening cost, both MCU supplies and any AP/sensor/driver costs inside the declared boundary. Report energy per attempt as well as per truth-verified success; an empty success set has an undefined ratio, not zero joules. Host-only UI/microphone power can be outside an MCU comparison boundary if explicitly excluded and held constant; that is not whole-house energy.

Measure RF conditions directly: quiet channel, AP idle, competing uplink/downlink, walls/distance, reconnect and channel changes. Existing application-frame muting is not an RF-coexistence experiment. Published Zigbee/Wi-Fi numbers cannot substitute for ESP-NOW measurements.

## Publication gates

1. Repair useful speech recognition and demonstrate the actual input-to-proposal-to-SafetyKernel path.
2. Return A, deploy both current images and rerun the paired suite; integrate the monitor with actual A transition ownership and clearly synthetic feedback first.
3. Complete representative live virtual-house traces and independent comparative baselines. The current B API is shadow verification only.
4. Add real low-voltage feedback and instruments if claiming electrical verification, energy, UPS or fail-operational power behavior.
5. Complete the closest-work comparison and choose claims/venue that match the evidence. No first-ever, best-model, physical-safety or acceptance guarantee is currently supported.

The research package is useful now as an executable starting point. Calling the project novel or publication-ready before these gates would overstate it.
