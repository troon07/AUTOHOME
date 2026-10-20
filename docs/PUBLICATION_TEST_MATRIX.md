# Publication test matrix and experimental protocol

Updated 20 October 2026. The platform is a native ESP-IDF, bounded-memory dual-MCU home controller with a deployed MultiNet6 speech model and a four-room synthetic-input demonstration. Model readiness is verified; a publication claiming useful edge AI still requires completed recognition and controlled comparisons. Both boards are currently unavailable. Functional prototype completion alone does not establish novelty, accuracy, energy savings or superiority over prior work.

There is no universal list that guarantees acceptance. This matrix covers the claims proposed for this project. PASS means the linked observation passed its stated test; PENDING means no supporting measurement exists. HOST_MEASURED tests of a simulator are not hardware measurements.

| ID | Claim / experiment | Procedure and required evidence | Current evidence | Remaining gate |
|---|---|---|---|---|
| T01 | Host regression | Run pytest with JUnit output; retain individual test results | HOST_MEASURED, `benchmarks/raw/host_tests.xml` | Rerun after relevant changes |
| T02 | C/Python wire compatibility | Independent golden bytes, CRC, endian/size/flags checks on host and both boards | Host tests plus board runtime self-tests | New schemas need equivalent vectors |
| T03 | Corruption rejection | Flip every bit for representative 0/1/16/44/128/220-byte payloads | HOST_MEASURED exhaustive single-bit campaign | RF corruption behavior separate |
| T04 | Credential/authentication boundary | Wrong/missing HTTP token; altered mandatory HMAC; wrong source MAC; known-answer HMAC test | Hardware runner + security boot test | Third-party penetration/review pending |
| T05 | Replay and duplicate handling | Re-send same command; modified same-ID payload; stale boot nonce | Hardware duplicate/wrong-boot injection; boot command-gate tests | Reboots, session rollover and adversarial corpus |
| T06 | ACK retry | Deliberately suppress one application ACK; verify two attempts, cached result, one execution | Hardware runner | Sweep burst loss and reorder rates |
| T07 | Outage behavior | Drop application frames for eight seconds; observe timeout, unknown telemetry after five seconds, control rejection, recovery | Hardware runner; intentional application fault | RF disconnection and power-loss tests |
| T08 | Safety limits | Dwell, manual hold, maximum run, invalid enum, registration, timer wrap | C mock-pin boot tests + hardware dwell denial | Observe physical low-voltage outputs |
| T09 | Home controls | Actual ACK state for light/fan ON/OFF; Home/Away/Sleep/All-off scenes; resume auto | GPIO-inhibited HIL cases | Load polarity, boot pulse and cycling tests |
| T10 | Timer/policy | Relative timer expiry/cancel, grace bounds, fan auto enable, restore defaults | GPIO-inhibited HIL + actual C/C++ automation checks | Real PIR transitions and fan response |
| T11 | HTTP bounds | Missing token, numeric overflow, oversized body, parser ambiguity and valid queueing | Hardware loopback HTTP cases | External clients, slowloris/header stress |
| T12 | Runtime memory | Internal free/min/largest; PSRAM free/largest; task stack margins; probe read/write and integrity | Hardware raw traces | Model/audio peak and allocation failures |
| T13 | Command RTT | At least 1,000 sequential idempotent OFF commands, raw timing and retry count; p50/p95/p99/CI | Hardware runner trials, if collected | Multiple independent sessions/boards/distances |
| T14 | Sustained stability | Concurrent workload with queue/heap/stack/reset monitoring; 24-hour minimum proposed pilot gate | Current run is a short soak only | 24–72 h with HTTP, radio, audio and model |
| T15 | Boot/power recovery | At least 100 power cycles; trace GPIO from power-up with logic analyzer; both output defaults OFF | PENDING | Actual GPIO waveform, brownout/recovery log |
| T16 | Relay/fan electrical behavior | Low-voltage DC only; polarity, isolation, flyback/protection, current and supply sag | PENDING | DMM/scope/current measurements |
| T17 | Offline operation | WAN blocked; phone browser, local commands and real speech work; capture network traffic | Local firmware uses no WAN service | Actual browser + microphone + WAN-block trial |
| T18 | Real occupancy labels | Independently annotated occupancy with timestamps and consent; single PIR only bedroom | PENDING | Do not label absence of PIR motion as vacancy |
| T19 | Learned predictor accuracy | Train/validation/test separated by days/users; fixed and statistical baselines; confusion, PR-AUC, calibration | PENDING; no trained predictor | Adequate representative labeled dataset |
| T20 | Speech model accuracy | Held-out speakers, accents, distances and noise; false accepts/hour, false rejects, command confusion | MultiNet6 resident/readiness verified; synthetic audio campaign completed but command recognition failed | Held-out human data and acoustic evaluation; current baseline has no AFE/WakeNet |
| T21 | True end-to-end speech timing | Acoustic onset/end and physical output on common measured timebase; separate capture/AFE/model/radio/actuation | PENDING | UART command RTT is not this metric |
| T22 | Single vs dual board | Same software, model, workload, quality and power measurement; disable B work for baseline rather than compare dissimilar tasks | PENDING | Implement one-board and two-board model workloads |
| T23 | Split model | Same trained model; enumerate split points; activation bytes, transfer, reassembly, inference accuracy, peak memory, latency | PENDING | No current tensor splitting |
| T24 | Energy/power | External shunt/power analyzer, integrate both boards and AP costs; report J/command and idle W | PENDING | On-time counters are not kWh |
| T25 | Automation utility | Compare occupied time with false OFF, unnecessary ON minutes, comfort violations and energy against timer baseline | PENDING | Ground truth occupancy and physical loads |
| T26 | Statistical independence | Multiple days, sessions and hardware units; report n at each hierarchy | One two-board bench setup | Confidence intervals cannot repair sampling bias |
| T27 | Ablation | Remove predictor, distributed work, encryption, queue bounds one at a time in a controlled bench build | PENDING | Never disable safety protections on connected loads |
| T28 | Reproducibility | Firmware/model/source hashes, versions, pins, clock/channel, raw traces, seeds and analysis scripts | Firmware hashes + raw HIL JSON/log/CSV | Add model/data provenance and release manifest |
| T29 | Threat/limitations | State key provisioning and physical access assumptions; document remaining network/flash exposure | Documented prototype boundary | Secure boot/flash encryption/HTTPS not implemented |
| T30 | User/browser operation | Phone/desktop layout, keyboard, stale state, busy/denied/timeout, scene partial denial | JS syntax + HTTP loopback tested | Actual phone browser usability trial |
| T31 | FreeRTOS application tasks | Static ControlTask/LinkTask, core/priority/period, job progress, stack minima and watchdog registration | RTOS-extended HIL runner | No hard WCET guarantee from observed maxima |
| T32 | Control isolation | Memory/HTTP/USB diagnostics concurrent with control and radio workers; queue/drop/deadline counters | RTOS-extended HIL runner | Add real audio/model/interrupt load |
| T33 | Watchdog recovery | Deliberately stall a subscribed task; confirm reset cause, recovery and safe GPIO waveform | Real A task-watchdog reset, reason 6, boot counter/policy/radio recovery observed; final policy read interrupted | Re-run complete recovery campaign and measure actual GPIO waveform |
| T34 | Driver/interface validation | PIR waveform/debounce, I2S/DMA buffers, future I2C/UART sensor error recovery | GPIO logic only; audio and extra sensors pending | Actual connected hardware and analyzer traces |

## Additional native/simulation gates

| ID | Claim / experiment | Current evidence | Remaining gate |
|---|---|---|---|
| T35 | Four-room synthetic input → A policy → virtual output | Actual C/C++ host tests + 23-case MCU HTTP loopback include state/expiry | Full external browser person-tour/manual/CSV session with both boards |
| T36 | Persistent boot identity/policy | CRC corruption tests; NVS survived induced watchdog reset | Final original-policy verification interrupted; corruption/power-cut campaigns pending |
| T37 | Explicit offline UI boundary | HOST_MEASURED preview rejects controls/audio with 503 and foreign origin with 403; movement/tour/dropout inspected | Actual phone/mobile usability and live browser session |
| T38 | Synthetic speech smoke corpus | 16 TTS clips + silence/noise generator, hashes and MCU runner prepared | Completed MCU run with every mismatch retained; not a substitute for T20 human accuracy |

Current results: 83 host tests, 81/81 controller check rows, 1,000/1,000 idempotent commands; [summary and limits](../benchmarks/reports/PROJECT_VALIDATION_SUMMARY.md). These counts are not independent experimental sample sizes.

## Experimental design

Use a preregistered primary hypothesis, such as: *a task allocation that places speech/dashboard on B and deterministic control on A improves the latency tail under concurrent work while preserving command accuracy and bounded memory*. Measure both boards' energy. A valid result can show that two boards cost more energy without useful latency improvement.

For the predictor, choose a measurable target: independently labeled occupancy or explicitly future PIR motion. Do not substitute one for the other. Compare fixed vacancy grace, a statistical estimator and a small quantized learned model on identical held-out days. Report false-off occupied minutes and unnecessary on-time, not accuracy alone. Quiet occupied people are a necessary difficult case.

For deployment comparisons, pin model files and operator implementations. Report every memory region, scratch allocation, task stack and activation peak. Reserve buffers before inference. Sweep concurrency and model sizes rather than filling all PSRAM merely to claim utilization.

Use at least several independent sessions per condition for pilot estimates, then choose sample size with a power analysis based on observed variance and the minimum useful effect. An arbitrary trial count does not prove significance. For paired latency/utility comparisons, pair equivalent inputs and bootstrap at the session/day level; use a block bootstrap if serial correlation is material. The runner's within-session IID bootstrap is descriptive and is labeled accordingly.

Do not compare this prototype's command ACK timings directly with published neural inference timings. Separate nominal latency, radio retry latency and failed transactions. Retain failures and report completion rate with a binomial interval. For a zero-failure run, the upper bound on failure probability remains nonzero.

## Reproduction commands

```powershell
python scripts/provision_local.py
python -m pytest tests/ -q --junitxml=benchmarks/raw/host_tests.xml
python -m platformio run -d native -e native_voice
python -m platformio run -d speech -e speech_intelligence
python scripts/resume_validation.py --deploy-staged
```

Run firmware builds sequentially because PlatformIO shares build bookkeeping. The active `--simulation-only --speech` HIL mode requires A to confirm GPIO inhibition before issuing commands. Older `--bare-boards` mode is historical and must not be used with loads. It saves incomplete/failed results too and exits nonzero on failure. Do not use that switch on a connected actuator installation.

Generated tables are `benchmarks/reports/HARDWARE_TEST_RESULTS.md`, `HARDWARE_TEST_RESULTS.csv`, `COMMAND_RTT_TRIALS.csv` and `hardware_statistics.json`. Firmware binaries contain bench credentials; distribute sources and reprovision each pair, rather than publishing an image containing shared secrets.

Generate standalone empirical latency/memory figures with `python scripts/plot_research.py <saved_hardware_json>`. Plots retain the trace filename and HARDWARE_MEASURED label. USB diagnostic recovery requests are reported separately from the device-measured radio/ACK timing.

## Deep-review contract and evidence experiments

| ID | Experiment | Current evidence | Remaining gate |
|---|---|---|---|
| T39 | Epoch/time/sample-bound evidence monitor | 23 actual C++ cases and B shadow cases, including a 1024-vector sweep | Integrate with A transition ownership; real feedback absent |
| T40 | Joint fault ablation and observability | 4096 constructed host conditions; zero necessary-contract violations, four refusals, two hidden-truth counterexamples | Strong prior-work baseline and independent faults; not a physical safety proof |
| T41 | Electrical-trace integration validity | Eight host checks for units, timing/gaps, NaN and known integrals | Real instrument/calibration/event traces; no measured joules |
| T42 | Bit-exact PCM and recognition | 18/18 PCM checksums matched on B;0/16 command hits, no command on two negatives | Repair recognition, then held-out human/noise/distance study |
| T43 | Current B-only operation | 32/32 API/shadow rows with exact image hash | Current A unavailable; prior paired timings are historical |
| T44 | Novelty and source coverage | Explicit closest-work matrix and narrowed empirical contribution candidate | Full-text/code/citation screening and fair comparative evidence |

See [the deep-review response](DEEP_REVIEW_RESPONSE.md) and [current results](../benchmarks/reports/PROJECT_VALIDATION_SUMMARY.md). Host tests now83 pass. No unresolved gate is closed by changing its source label.
