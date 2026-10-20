# Response to the deep research analysis

20 October 2026. The supplied `ClawGrid_Deep_Research_Literature_Review.docx` is a reference, unchanged. Its proposed mains installation is not authorization to activate mains or security eFuses. The current user setup has **Node B on COM7 only**, nothing wired; Node A is still unavailable.

## What changed

The review is strongest when it asks for measurable fault behavior and separates electrical truth from a software acknowledgement. I implemented a fixed-memory observation monitor, a reproducible fault/ablation matrix, and an electrical-trace analysis tool. Native A and speech B compile. B was flashed and its API/shadow suite passed 32/32 check rows. The host regression has 83 passing tests. The original 81/81 paired controller session belongs to earlier image hashes and is retained separately.

The new monitor is a **shadow component**, not yet an enforcement layer in A's live device registry. It never writes GPIO. Its OFF request is only a proposal that a future owner must route through SafetyKernel; it cannot override dwell, repair welded contacts, provide an emergency stop or prove electrical isolation.

## Concrete changes and evidence

| Change | Evidence | Limits |
|---|---|---|
| State monitor with unknown/settling/verified/fault states | Actual firmware header and 23 cases on COM7 | Synthetic evidence; no contact/current hardware |
| Freshness, boot/command identity, sample order and nonrenewable evidence | Host/MCU cases include future/stale/duplicate samples, time wrap, gaps and late agreement | Trusted monotonic clock and authenticated upstream transport assumed |
| Fault latch and explicit OFF recovery | Contradiction latches; repeated ON cannot extend a pending deadline; recovery requires fresh distinct OFF evidence | Not connected to physical output inhibition yet |
| Resource footprint | B reports 60 B per monitor instance; one 23-case self-test execution took 375 us | Self-test aggregate, not normal transition latency, WCET or a distribution |
| Finite ablations | 4096 designed conditions, actual C++ monitor, raw CSV and summary | HOST_MEASURED execution on SIMULATED inputs; weak ablations are not SOTA baselines |
| Energy analysis | DC V(t)I(t) trapezoidal integration; rejects missing/gapped/reversed/NaN samples | No instrument trace currently exists; no energy-saving figure generated |
| Speech transport | Longer bounded diagnostic transfer budget; checksum equality verifies submitted PCM bytes on B | Recognition still fails; transport success is not speech accuracy |

Raw single-board evidence: `benchmarks/raw/hardware/single_b_20261020T080754Z.json`. It includes the exact B firmware hash. The monitor had all 23 pass bits set. Its test inputs were SIMULATED; its execution and API memory readings were HARDWARE_MEASURED. The property sweep inside one case contains 1024 combinations; it is not 1024 independent deployments.

## Fault-matrix result and its difficult counterexample

| Ablation | Verification assertions | Necessary-contract violations | Eligible cases refused | False physical assertions when modeled feedback lies |
|---|---:|---:|---:|---:|
| ACK treated as verification | 4096 | 4088 | 0 | 2048 |
| Feedback values only | 256 | 248 | 0 | 128 |
| Fresh, identity-bound feedback | 32 | 24 | 0 | 16 |
| Full observation monitor | 4 | 0 | 4 | 2 |

These are counts in a deliberately balanced Cartesian fault model, not household failure rates or measured accuracy. The necessary-contract oracle admits eight cases; the monitor verifies four and refuses four because the older evidence lease expires between observations. That conservative availability cost must be reported alongside its zero contract violations.

More importantly, **two false physical assertions remain** when both apparently separate feedback modalities lie consistently. Agreement cannot establish physical truth without justified sensor accuracy, calibration and common-cause assumptions. This corrects the review's implication that three-way agreement automatically proves safe electrical control. No software monitor can infer a hidden welded-contact state from two identical false measurements alone.

## Speech is still an unresolved functional issue

A completed B-only raw-PCM corpus matched none of the 16 TTS commands, while silence and white noise produced no detection. All 18 submitted clip checksums matched on B. This localizes the current issue beyond input transport; it does not establish the model/AFE cause. Decoder diagnostics are now bounded local ASCII text. The tiny two-negative set cannot estimate false accepts/hour, and no human/acoustic performance or best-model claim is made.

This defect is a priority before any paper claims useful offline voice. The reviewed 5 ms single-keyword result on a different MCU/dataset is not an appropriate numerical baseline for our multiword command recognizer or acoustic-to-output pipeline.

## Corrections to the analysis and novelty claim

The review omits substantial runtime-assurance literature. [Black-Box Simplex](https://arxiv.org/abs/2102.12981) already separates unverified control from runtime safety checks. [NASA's formal RTA framework](https://ntrs.nasa.gov/citations/20240006522) provides a formal construction approach. Neither AI mediation nor a safety state machine is a new invention here.

Freshness contracts also have close prior art: [Mission-Aware Attestation Envelopes](https://arxiv.org/abs/2610.08771), submitted 6 October 2026, explicitly combines integrity, evidence age and decision deadlines in HIL experiments. [Split CNN Inference on Networked Microcontrollers](https://arxiv.org/abs/2605.09357) already covers collaborative MCU model execution. Two S3 boards, leases or split inference alone cannot support a novelty claim.

The narrower candidate is **a reproducible MCU benchmark of fault observability and freshness–availability tradeoffs in offline home control under concurrent inference and correlated faults**. The contribution would be the defined fault model, executable contracts, resource measurements and comparative results, with explicit unobservable failure cases. It is a candidate empirical systems contribution, not established theoretical novelty or a guaranteed publication.

The uploaded Zigbee study uses PDR to mean **packet drop rate**, not delivery ratio; write out the term wherever that numerical comparison is reused. Its numbers are not ESP-NOW measurements. The source appendix's DR handles are previous research-panel identifiers; they must be replaced with ordinary references before submission. Recovered links do not automatically verify every nearby numerical statement. Publisher access to some full texts was rate-limited, so unverified values are not promoted as confirmed facts.

## Complete issue register

The accompanying [30-row resolution CSV](../benchmarks/reports/DEEP_REVIEW_RESOLUTION.csv) maps every major recommendation to implementation/evidence and a remaining gate. Hardware-dependent items are explicit: metering, auxiliary feedback, physical safety, UPS, electrical energy, acoustic labels, RF coexistence, power cycling and secure provisioning. Possessing two relays does not supply those measurements. Signed OTA, database services, cooperative shedding and general LLM policies remain absent or partial rather than being relabeled as complete.

[Monitor specification](EVIDENCE_MONITOR_SPEC.md) · [Novelty and experimental plan](NOVELTY_CASE.md) · [Ablation CSV](../benchmarks/reports/EVIDENCE_ABLATIONS.csv) · [Single-board cases](../benchmarks/reports/SINGLE_BOARD_B_RESULTS.csv).
