# Data collection for the next learned-model experiment

No occupancy dataset has yet been collected from this prototype. Current hardware traces contain bare-board state and diagnostics, not labeled human activity.

## Ground truth and room scope

Collect only the assigned bedroom PIR stream. Record an independent observer's occupancy label with consent and a clear annotation method. A quiet seated person must remain labeled occupied even when PIR motion is absent. Do not derive the label from the PIR, light/fan state or the automatic grace timer.

Use the CSV template `benchmarks/templates/occupancy_labels.csv`. Each row records UTC time, session/room, board uptime, debounced motion, independently observed occupancy, label method and annotation confidence. Keep unknown labels empty. Store any identifying information outside the research CSV and apply the project's agreed retention policy.

The firmware emits `motion_event` changes over USB; they are not fixed-rate samples. For a fixed-rate training set, reconstruct last-known motion only across valid continuous capture intervals and retain gap flags. Do not silently fill resets/disconnections. Pair that stream with independently timestamped labels and document the alignment tolerance. A sampled HIGH value can persist due to the PIR's own hold interval; it is not a count of people.

## Timing and leakage

Host receipt time and board uptime use different clocks. Align within each boot/session and record observed clock drift. Existing command RTT is measured on B's one clock; do not subtract A's `micros()` from B's to invent one-way latency. Physical end-to-end speech/output measurement requires a common acquisition timebase.

Choose all features from the causal past. Split train, validation and test by complete day/session/person before constructing overlapping windows. Fit normalization and thresholds on training data only; choose model/hyperparameters using validation only. Keep the final test labels unused until selection is complete.

Compare a fixed 120-second timer, a validation-selected statistical timer and a compact learned predictor on the same intervals. Include long quiet occupancy, brief visits, entry/exit, pet/curtain false triggers if relevant, and sensor warm-up/gaps. Report confusion and calibration alongside false-off occupied minutes and unnecessary ON time. Single-room observational data do not establish multi-room localization or population generalization.

## Speech and energy

For speech, retain intended command labels, speaker/session group, distance, room/noise condition, model/config hash and acquisition settings. Include silence and non-command speech for false-accept evaluation. Use held-out speakers; do not count repeated recordings of the same utterance as independent people.

For energy, use an external instrument and record supply voltage, sample rate, calibration and both boards' consumption. Keep idle, audio listening, inference, radio and actual output-load contributions separate. Firmware on-time estimates cannot replace this measurement.

See [publication matrix](PUBLICATION_TEST_MATRIX.md) for the claim-specific tests and [literature review](LITERATURE_REVIEW.md) for public dataset candidates and their domain limitations.
