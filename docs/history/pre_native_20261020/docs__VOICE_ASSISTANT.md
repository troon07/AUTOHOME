# Text assistant and speech integration plan

The embedded dashboard currently previews explicit text commands such as `turn on bedroom light`. Missing room/device/action, negation, ambiguity and unknown words are rejected. Preview does not execute an action. The user can confirm a valid bedroom light/fan proposal, which then travels through the authenticated command and SafetyKernel path.

There is no learned language model in the current parser and no microphone capture in the firmware. Parser duration measures deterministic text processing only.

The proposed offline speech path is ESP-SR AFE → WakeNet → MultiNet, selected and pinned after microphone/SDK compatibility checks. Use an ESP-SKAINET reference example as the integration starting point. Measure false accepts, false rejects, command accuracy, latency, memory and energy on the actual microphone. A browser speech service cannot establish an offline speech claim.

See [research direction](RESEARCH_DIRECTION.md), [model evaluation](MODEL_EVALUATION.md) and [publication test matrix](PUBLICATION_TEST_MATRIX.md).
