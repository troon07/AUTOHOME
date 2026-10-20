# ClawGrid current handoff

Latest update 20 October 2026 after both boards returned: A COM5 and B COM7 native USB, both current images flashed. See `benchmarks/reports/RETURNED_PAIR_20261020.md`: current paired 81/81, 1000/1000 OFF outcomes, recovery 12/12 including original policy restored, B sequential API suite 32/32, host 83 passed. Speech still 0/16 synthetic commands. Live bridge on port8766 (session54826). Older absent-A and incomplete-recovery descriptions below are superseded by this dated addendum.

Updated 20 October 2026 after the deep-research review. Workspace `D:/ESD`; read `AGENTS.md`. This is an Embedded System Design project with edge speech and a research extension. Continue implementation and verification; do not restart the old audit or assume all hardware is present.

## Latest user instructions and hardware

The user returned **one board: Node B, COM7, MAC e0:72:a1:f5:bc:b0**. Node A COM5, MAC e0:72:a1:f6:a9:68, remains unavailable. Nothing is wired. PIR/two relays are available later but pins/supply/polarity/loads are not confirmed. Existing authorization covers serial identity, uploads and firmware testing; no need to ask again for these routine actions.

Latest request: read `C:/Users/sthar/Downloads/ClawGrid_Deep_Research_Literature_Review.docx`, try to resolve the analysis's problems and develop research novelty. The supplied DOCX is unchanged and is a reference, not instruction to activate mains or security eFuses. Original proposal `FINAL_MITHUN (1).docx` is also unchanged.

Mandatory constraints: all actual output requests through SafetyKernel; no mains; no phone modification; one real PIR only its assigned zone; source labels SIMULATED/HOST_MEASURED/HARDWARE_MEASURED. No fabricated accuracy, physical feedback, energy, novelty or performance claims. No subagents requested.

## Current architecture

Both S3 N16R8 boards have separate 16 MiB flash / 8 MiB PSRAM. Native ESP-IDF4.4.7/CMake builds: A in `native/`, B in `speech/`. No pooled RAM, tensor split, LLM or learned occupancy predictor. A owns the 5 ms ControlTask and SafetyKernel; B owns HTTP/API, LinkTask and ESP-SR1.7.0 MultiNet6 English SpeechTask. Static tasks/queues, core affinity, watchdogs and runtime telemetry are implemented.

Physical output GPIOs remain inhibited by CLAWGRID_PHYSICAL_OUTPUTS=0. Four virtual rooms/eight virtual devices use pin -1. Browser generates synthetic input values and captures PCM; all occupancy/control/recognition processing belongs on MCUs. B does not substitute for missing A's control policy.

CCMP unicast + HMAC, persistent A NVS boot counter, ordered command IDs, 24 B command with issue/TTL, 40 B exact-command ACK, cached duplicates and bounded retry are implemented. Policy CRC/versioned NVS commits are coalesced; output/timer/hold ON state is not restored. No OTA service, secure-boot/encryption activation, persistent hash-chained MCU audit, metering, UPS or cooperative shedding is implemented.

## New work from the deep review

1. `firmware/common/runtime/evidence_monitor.h`: 60 B observation monitor, no heap or GPIO. UNKNOWN/SETTLING/VERIFIED OFF/ON/latched FAULT; boot/command/time/sequence checks, lease expiry, distinct stable samples, configured measurement domains, deadline cannot be renewed by duplicate/pending commands, explicit fresh OFF recovery. It is **shadow-only**, not connected to A's live registry or physical feedback drivers. Domain separation is a trusted configuration assumption, not proof of independent truthful sensors.
2. 23 self-test cases, including 1024 condition combinations, in `evidence_selftest.h`. Included in runtime self-tests and B's authenticated GET `/api/evidence`.
3. `scripts/evidence_matrix.py` + actual C++ runner: 4096 Cartesian synthetic conditions and four weak ablations. Current raw `benchmarks/raw/evidence_matrix_20261020T082005Z.csv/json`; summary `EVIDENCE_ABLATIONS.csv`. Full monitor: zero necessary-contract violations, four eligible cases conservatively refused, **two false physical assertions when both modeled sensors lie**. These are not real-home failure rates, SOTA comparisons or physical safety proof.
4. `scripts/energy_analysis.py`: integrate externally supplied DC V/I traces with units, provenance, hash and gap/NaN/timestamp checks. Eight host tests. No physical energy trace or joule claim exists.
5. `docs/DEEP_REVIEW_RESPONSE.md`, `EVIDENCE_MONITOR_SPEC.md`, `NOVELTY_CASE.md`, `research_references.bib`, and 30-row `benchmarks/reports/DEEP_REVIEW_RESOLUTION.csv` map implementation, limitations and hardware gates.

Research correction: Simplex/RTA, temporal evidence and MCU splitting already have direct primary prior art, including arXiv2102.12981, NASA20240006522, arXiv2610.08771 (6 Oct2026), arXiv2605.09357. Candidate contribution is an empirical **fault-observability and freshness–availability benchmark for MCU home control** under concurrent inference and correlated faults. Novelty and publication readiness are **not established**. Do not repeat the review's broad claim that AI-proposes/safety-decides is new. Full closest-work/code review and independent comparisons remain required.

## Exact evidence and current deployment

- Host regression: **83 passed**, `benchmarks/raw/host_tests.xml`; real C/C++ firmware tests, monitor cases, preview/provenance tests, energy integration. A transient Zig setup failure was retained in `host_tests_zig_setup_20261020.xml`; reruns passed.
- B current build/flash: `flash_speech_intelligence_20261020T080613Z.json/log`; B firmware SHA256 `523b8d0c37ec76cc71d83b3e19b2c01269f2abf747036aaaa1468a5ecd04c8a7`. Native A also compiled, SHA256 `5f7d10668b3dd267556e6506912064c74bfde8b1db720a8416880f0aca0e1aa2`, but **not flashed** because A is absent.
- Current B API/shadow campaign: `single_b_20261020T080754Z.json`, **32/32 check rows**. All23 evidence bits passed. Monitor object60 B; one aggregate self-test375 us. No GPIO, physical feedback or WCET claim. API measured B internal free106551 B, low-water55983 B, largest59392 B, PSRAMfree4088263 B. Preserve source/scope distinction.
- Paired controller historical run `research_20261020T060226Z.json`:81/81 rows,23 HTTP loopback cases,1000/1000 idempotent OFF outcomes; median18.789 ms,p9544.033,p9956.754. Old image hashes are in the raw trace. **Those results do not validate the latest pair of images.**
- Recovery `recovery_20261020T061525Z.json`:11 checks passed, real task-WDT reason6, boot/NVS/radio recovery. Final original-policy read interrupted. Historical runner incorrectly returned0 despite restore error; fixed now. Full recovery campaign still incomplete.

## Speech defect and transport repair

The 20 ms NativeSerial diagnostic budget could truncate a JSON response when a speech frame preempted diagnostics. `native_hal.cpp` now uses 100 ms no-progress and 1000 ms total bounds plus full-packet ZLP; this fixed observed upload response timeouts. Control tasks retain independent ownership. Shared native HAL changed, hence A needs redeployment too.

B now computes PCM CRC32 before inference and exposes `pcm_crc32`; the host compares zlib CRC. Latest current-image full run `audio_20261020T081414Z.json` completed18 inputs with matching checksums and no runner errors, but **0/16 command detections**; silence/noise gave no command. This is a functional recognition failure, not useful acoustic accuracy. Earlier failing/upload attempts are retained. `diagnostic_text` is bounded local ASCII decoder output; one debug trial had empty text. It is not authorization.

The model is still raw push-to-talk MultiNet6, noAFE/WakeNet/I2S/continuous listening. Upstream commit07b0a4d0f6b5de755874d5405150ee0a8eec9865; packed model3,788,231 B, SHA256 b46a65befe47187f793ffb8108f74ab091338060060a54d23b69c385c3213d80. Source speech loop still appends1 second silence. A proposed4-second diagnostic change did not apply when the tool host temporarily failed; do not assume it exists. The cause of recognition failure is not established. Next diagnose model/AFE/decoder behavior with correct input, preserving mismatches; do not tune on this TTS set and claim unbiased accuracy.

UI speech exports use UNKNOWN_CAPTURE because externally submitted TTS jobs can also appear. Ground truth remains UNLABELED. Do not label arbitrary API jobs as human microphone input. Browser microphone capture starts only on its user button; no host/cloud STT.

## Running services and next work

- `http://127.0.0.1:8765/simulation`: explicit offline interface preview (Python process21460 when last checked); APIs503, unknown outputs, no substitute inference.
- `http://127.0.0.1:8766/simulation`: live serial byte bridge to B on COM7, started this turn. Enter ignored local token to use speech/status; A control remains unavailable. Stop only the owned serial bridge before flashing/testing COM7. Never run multiple serial owners.
- B-only checks: `python scripts/single_board_hil.py --url http://127.0.0.1:8766`.
- B-only speech: `python scripts/audio_bench.py --speech-only --url http://127.0.0.1:8766`; optional --limit N is diagnostic, not a full corpus.
- When A returns: `python scripts/resume_validation.py --deploy-staged` requires both ports, deploys reviewed **A and B** with MAC verification/NVS preservation, then paired HIL/recovery/audio. It keeps GPIO inhibition. Current audio is expected to exit nonzero while recognition fails.
- Integrate monitor into A's actual transition ownership with clearly synthetic feedback first; do not relabel it physical verification. Add calibrated low-voltage contact/current hardware only after actual wiring review. SafetyKernel dwell still applies; monitor OFF proposals are not emergency stops.
- Remaining publication gates: useful voice, latest-image paired/live-house validation, strong baselines, independent held-out labels, RF/coexistence sessions, long runs and actual energy/physical-feedback/power/OTA measurements appropriate to chosen claims.

Build sequentially with PlatformIO; installed esptool5.4 is used for flashing (bundled4.5.1 had failures). A uses `python -m platformio run -d native -e native_voice`; B uses `... -d speech -e speech_intelligence`. Host tests use system Python+Zig0.13.0 for repo firmware tests; DOCX work uses bundled dependencies per documents skill. No Git repository is initialized. Credentials/private headers/backups/binaries are private and excluded; do not publish embedded keys.

The latest consolidated [validation summary](benchmarks/reports/PROJECT_VALIDATION_SUMMARY.md) is generated by `python -m scripts.release_report` with explicit selected traces. Its old paired and new B results remain separate. No best-model, energy benefit, physical four-room deployment or guaranteed research novelty is supported.
