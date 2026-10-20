# Current validation summary — 20 October 2026

Node B has returned on COM7; Node A is still unavailable. The current B build passed a single-board shadow/API campaign, while the paired controller timings below belong to the earlier explicitly hashed session. Port 8765 remains the offline house preview; port 8766 is a live byte bridge to B. Neither host service substitutes control or inference for the missing A.

## Completed and incomplete checks

| Area | Source | Result | Scope |
|---|---|---|---|
| Host regression | HOST_MEASURED | 83 passed | Actual firmware C/C++ tests included |
| Previous paired controller check rows | HARDWARE_MEASURED | 81/81 PASS | Earlier image hashes; current paired regression pending |
| Sequential command outcomes | HARDWARE_MEASURED | 1000/1000 | Idempotent OFF; GPIO inhibited |
| Command ACK median / p95 / p99 (ms) | HARDWARE_MEASURED | 18.789 / 44.033 / 56.754 | Device RTT, not speech/physical actuation latency |
| HTTP loopback | HARDWARE_MEASURED | 23/23 PASS | Includes virtual-house input/output/expiry cases |
| Watchdog recovery | HARDWARE_MEASURED | 11 recorded checks PASS; campaign incomplete | Reset cause, boot counter and saved policy passed; final restored-policy read interrupted |
| Speech model readiness | HARDWARE_MEASURED | PASS | MultiNet6 English loaded; recognition currently fails |
| Synthetic speech benchmark | SIMULATED input / HARDWARE_MEASURED processing | Completed: 0/16 command matches; 0/2 negative false accepts | All 18 PCM checksums matched; no human accuracy result |
| House interface interaction | HOST_MEASURED | Layout/movement/tour/dropout inspected offline | No board decisions or fabricated outputs in preview |
| Human microphone / acoustic evaluation | NOT_MEASURED | PENDING | Requires returned board and labeled human recordings |
| Physical sensors / relays / energy | NOT_MEASURED | PENDING | Nothing wired; physical outputs inhibited |
| Current Node B campaign | HARDWARE_MEASURED | 32/32 rows passed | Shadow monitor/API tests; no actuation |
| Evidence-monitor fault matrix | SIMULATED input / HOST_MEASURED execution | 4096 conditions; zero necessary-contract violations | Four eligible cases refused; two physical false assertions under jointly lying sensors |
| B update | HARDWARE_MEASURED | Flashed; single-board tested | Transport/provenance/PCM diagnostics and shadow monitor; A remains unavailable |

## Historical paired controller run

Trace: [raw JSON](../raw/hardware/research_20261020T060226Z.json), [serial log](../raw/hardware/research_20261020T060226Z.log). Observation lasted approximately 194.6 seconds, including a 60-second soak. One pair of boards, one session; speech model resident but no inference workload during these command trials. No sensor/relay/load was connected.

ACK mean: 22.589 ms; maximum: 91.841 ms. See [individual cases](HARDWARE_TEST_RESULTS.md), [1,000 trials](COMMAND_RTT_TRIALS.csv) and [statistics](hardware_statistics.json) for descriptive bootstrap/Wilson intervals. These do not establish population reliability or a speedup over a controlled baseline.

### RTOS observations — HARDWARE_MEASURED

| Node | Task | Max execution µs | Max release lateness µs | Observed target misses | Min free stack B |
|---|---|---|---|---|---|
| A | ControlTask | 420 | 144 | 0 | 4016 |
| A | LinkTask | 1511 | 5 | 0 | 4096 |
| B | LinkTask | 1247 | 2 | 0 | 3244 |

Five milliseconds is the design target; observed maxima are not a WCET proof. Wi-Fi/SDK tasks still share CPU time.

### Memory observations — HARDWARE_MEASURED, bytes

| Node | Min sampled internal free | SDK internal low-water since boot | Min largest internal block | Min sampled PSRAM free | Min largest PSRAM block |
|---|---|---|---|---|---|
| A | 190075 | 180851 | 151552 | 8386111 | 8257536 |
| B | 104655 | 55459 | 59392 | 4089807 | 3866624 |

The SDK low-water measurement includes earlier work in that boot; it is different from the lowest periodically sampled free-memory reading. In particular B reached **55,459 B** internal low-water with the model resident; do not claim a continuously maintained 64 KiB reserve. These measurements do not include a completed simultaneous audio/control stress campaign. Each board has its own 8 MiB PSRAM.

## Recovery and failed attempts

[Recovery raw evidence](../raw/hardware/recovery_20261020T061525Z.json) records a real task-watchdog reset (reason 6), increased persistent boot counter, preserved Away/grace policy, OFF software outputs and resumed acknowledged radio commands. Eleven recorded checks passed. The final original-policy verification was interrupted by a USB transport exception, so the full recovery campaign is **incomplete**. Its old runner returned zero despite this restore exception; that exit-code bug is now fixed. The historical raw file remains unchanged.

The earlier native run `research_20261020T055435Z.json` failed two stale-state checks. A full 64-byte USB packet delayed its fault acknowledgement; a ZLP fix was applied and the current 81-row run passed. Earlier export/uploader failures, journals and audio transport timeouts are retained. No unsuccessful attempt is pooled into a passing latency dataset.

The earlier audio timeout was reproduced on returned B. A 20 ms diagnostic send budget could truncate JSON while SpeechTask preempted diagnostics. A bounded progress/total budget fixed the observed transport failure. B now checksums PCM before inference. The latest [18-input speech run](../raw/hardware/audio_20261020T081414Z.json) completed without runner errors and with matching input checksums, but **detected none of the 16 command clips**. Silence/noise returned no command. This is a failed functional smoke result, not human accuracy or false-accepts/hour. All previous failures remain retained.

## Current B shadow evidence

[Current raw B trace](../raw/hardware/single_b_20261020T080754Z.json) passed 32 rows, including all 23 monitor cases. One case sweeps 1024 combinations. B reports a 60 B monitor object and 375 us for that aggregate self-test; this is not per-transition timing or WCET. The monitor is not yet integrated into A's output registry, and synthetic feedback never becomes physical verification. See the [deep-review response](../../docs/DEEP_REVIEW_RESPONSE.md), [novelty dossier](../../docs/NOVELTY_CASE.md) and [ablation CSV](EVIDENCE_ABLATIONS.csv).

## Tested firmware/model SHA256

- `native_voice`: `edb0ff713a1773576b3089535120e33fe0a368be3cd44ab5f41cd73e41899af4`
- `speech_intelligence`: `b127f5d99a7943ddd76a7305261ac0278584545c746c14942abf705f61a37e91`
- `speech_models`: `b46a65befe47187f793ffb8108f74ab091338060060a54d23b69c385c3213d80`

## Resume with both bare boards

```powershell
python scripts/resume_validation.py --deploy-staged
```

The current B image SHA256 is `523b8d0c37ec76cc71d83b3e19b2c01269f2abf747036aaaa1468a5ecd04c8a7`; it is flashed and matches the selected single-board trace: `True`. Its static linker use is 84,168 B RAM and 1,055,237 B application flash. Current A compiled with SHA256 `5f7d10668b3dd267556e6506912064c74bfde8b1db720a8416880f0aca0e1aa2` but is not flashed because A is absent; static linker use is 63,128 B RAM and 649,077 B application flash. These are HOST_MEASURED build sizes, not runtime heap peaks. The historical paired hashes above remain unchanged.

The resume command requires both boards and deploys reviewed A and B images/model with MAC verification and preserved NVS, then runs GPIO-inhibited paired controller/recovery and synthetic speech suites. It does not enable outputs or modify the phone. Without --deploy-staged it refuses a build/last-flash mismatch. Speech currently fails recognition, so expect a nonzero audio result until that defect is repaired. Hardware-dependent and publication-dependent gates remain listed in [the publication matrix](../../docs/PUBLICATION_TEST_MATRIX.md).
