# Requirements and evidence traceability

Current date: 20 October 2026. Both boards temporarily unavailable; no sensors/relays/loads wired. Physical outputs inhibited. Native ESP-IDF/FreeRTOS and the virtual-house MCU backend are implemented. B has a pinned MultiNet6 English model; complete speech/audio validation is pending.

Authoritative traceability: [proposal-to-execution table](PROPOSAL_VS_IMPLEMENTATION.md), [feature/API table](HOME_AUTOMATION_FEATURES.md), [publication test matrix](PUBLICATION_TEST_MATRIX.md) and [current measured results](../benchmarks/reports/PROJECT_VALIDATION_SUMMARY.md).

83 host tests pass, including compiled actual C/C++ logic and offline-preview boundaries. One native hardware session passed 81 controller rows and 1,000 command outcomes. Model readiness passed. Recovery final verification and synthetic/human speech campaigns are incomplete. These scopes must remain separate.

SafetyKernel is mandatory for every proposed action; mains prohibited; phone is a standard browser client; one real PIR covers only its assigned zone. Browser position and synthetic presence are not physical localization. No memory pooling, tensor split, learned occupancy predictor, energy savings or best-model result is established.
