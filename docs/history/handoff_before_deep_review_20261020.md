# ClawGrid: self-contained handoff for the next chat

Status: 20 October 2026. Workspace `D:/ESD`. Read this and `AGENTS.md` before acting. Current user's instruction: **both ESP32-S3 boards are temporarily unavailable; complete other work meanwhile**. Stop serial access until the user reports they have returned. Nothing is wired. A PIR and two relays exist but their wiring/polarity/supply are not confirmed.

## Objective and constraints

This is an Embedded System Design course project, with useful local home lighting/fan automation first and edge/distributed AI as the research extension. The user wants a virtual house with a movable person, editable synthetic sensors, laptop/phone microphone input, all recognition/control processing on ESP32s, monitoring, publication tests/tables, literature review and proposal comparison.

Mandatory: SafetyKernel owns all outputs; low-voltage only, no mains; phone remains a standard browser client; one real PIR covers one assigned zone; distinguish SIMULATED, HOST_MEASURED and HARDWARE_MEASURED. Never invent timing/memory/accuracy/energy/physical-feedback results. Existing authorization covers COM5/COM7 identity, firmware uploads and tests when available; do not ask again solely for routine serial access. Physical output enabling is not authorized by the submitted proposal and needs actual wiring facts.

Proposal reference, unchanged: `C:/Users/sthar/Downloads/FINAL_MITHUN (1).docx`. Its four physical room controllers, ESP-IDF/RTOS, offline speech, optional Pi NLU, metering/cooperative shedding, UPS, auxiliary/current/voltage verification, MQTT/database, signed OTA and production security exceed the current virtual prototype. [Current comparison](docs/PROPOSAL_VS_IMPLEMENTATION.md) identifies implemented/partial/pending work.

## Actual architecture and firmware

- A, recorded COM5, MAC e0:72:a1:f6:a9:68: native ESP-IDF control/SafetyKernel, real-bedroom PIR software policy and four virtual-room policies.
- B, recorded COM7, MAC e0:72:a1:f5:bc:b0: native ESP-IDF HTTP/API, transactions and **ESP-SR 1.7.0 MultiNet6 English** model.
- Both ESP32-S3 v0.2 N16R8: 16 MiB flash, 8 MiB PSRAM **each**, independent memory. No tensor splitting, pooled RAM, LLM or learned occupancy predictor.
- Current firmware defaults `CLAWGRID_PHYSICAL_OUTPUTS=0`; physical-bedroom device slots use pin -1, eight virtual devices also pin -1. Nothing is wired.
- Static A ControlTask core1 priority5, LinkTask core0 priority3; B LinkTask core0 priority3, SpeechTask core1 priority2; Diagnostics core1 priority1; SDK HTTP/Wi-Fi tasks remain. See `docs/EMBEDDED_SYSTEM_DESIGN.md`.
- Every request goes through A SafetyKernel: whitelist, 3 s dwell, 5 min manual hold, 4 h max run. Bedroom scenes/timers/resume/grace; four virtual rooms use occupied/lux/temp thresholds.
- Browser draws/synthesizes raw inputs and formats captured PCM; it performs no speech/occupancy/control inference. USB bridge only transports bytes. Unknown MCU state must never be replaced with host decisions.
- Virtual inputs: 36 B SIM schema1, A boot/sequence/uptime, four flags/temp/lux entries. State32 B SSM. Input age/lease bounded, missing sensors unknown. One real PIR tracker is separate.
- CCMP paired unicast plus mandatory truncated HMAC; persistent boot-bound ordered commands, 24 B command with issued-at/TTL, 40 B exact-payload ACK, cached duplicates, 4 attempts at 300 ms. Status6 expired. Both boards must use compatible payload lengths.
- Versioned explicit-byte CRC NVS record saves policy/Away/grace after5 s quiet; persistent 32-bit boot counter before control. No NVS autoerase. Outputs/timers/holds are not restored ON. Audit32 commands remains volatile. No hash-chained database.
- Model image3,788,231 B;6 MiB model partition. Pinned upstream ESP-SR commit `07b0a4d0f6b5de755874d5405150ee0a8eec9865`; esp-dsp1.2.1. Downloader verifies Git blob hashes and records SHA256. Third-party source/deps ignored locally.
- Speech: one128000 B PSRAM clip, static12288 B internal task stack, mono16k PCM16LE,16 commands, raw push-to-talk. NoAFE/WakeNet/I2S/continuous listening. Explicit confirmation proposes virtual command; no automatic actuation.

## Verified evidence (do not overwrite historical files)

1. Current host suite: **52 passed**, `benchmarks/raw/host_tests.xml`. Includes6 tests compiling actual firmware C/C++ through Zig 0.13.0, 3 flash-provenance tests and 3 offline-preview tests (503/no fake outputs/foreign-origin403). Host counts are not hardware experiments.
2. Complete native controller session: `benchmarks/raw/hardware/research_20261020T060226Z.json` + `.log`: **81/81 rows**,23/23 HTTP loopback,1,000/1,000 idempotent OFF outcomes; no errors. 194.6 s including60 s soak; model resident but no audio inference workload during command timing.
3. ACK median18.789 ms,p9544.033 ms,p9956.754 ms,mean22.589 ms,max91.841 ms. Device command/ACK RTT, not acoustic/output latency or proof of speedup.
4. A ControlTask/LinkTask and B LinkTask max execution420/1511/1247 µs; zero recorded5 ms target misses; min stackfree4016/4096/3244 B. Not WCET proof.
5. Min sampled internal free: A 190075 B, B 104655 B; SDK low-water: A 180851 B, B 55459 B. PSRAM free: A 8386111 B, B 4089807 B. B does **not** maintain 64 KiB internal low-water headroom. Current generic 2 MiB ModelMemory arena is unused by ESP-SR; do not conflate allocations.
6. Recovery `recovery_20261020T061525Z.json`:11 recorded checks PASS, actual task-WDT reason6, advancedboot,NVS retained,OFFsoftwarestate,radio ACK recovered. Final original-policy read interrupted by serial exception; **campaign incomplete**. Old runner incorrectly returned0 despite restore exception; now fixed to appendFAIL/returnnonzero. Do not edit raw historical evidence to make it pass.
7. Speech readiness passed on B. `audio_*.json` attempts retained USB/API timeouts, with no completed accuracy dataset. Some PCM jobs completed internally with ID0, but there is no retained, correctly bound full recognition evaluation; do not report a match rate. Boards were removed during this period. Diagnose actual transport versus disconnection before attributing failure.
8. Earlier native `research_20261020T055435Z.json` failed stale tests. Full64-byte USB fault ACK delayed Windows delivery; NativeSerial now emitsZLP after full finalpacket. Targeted ACK16 ms and final81-row session passed. Export failures/uploader errors/journals retained. Old Arduino report archived at `benchmarks/reports/native_migration_baseline/`; other snapshots in usb_transport_failure/pre_rtos_baseline. Historical docs in `docs/history/pre_native_20261020/`.

Tested firmware SHA256:
- A native_voice: edb0ff713a1773576b3089535120e33fe0a368be3cd44ab5f41cd73e41899af4
- B speech_intelligence: b127f5d99a7943ddd76a7305261ac0278584545c746c14942abf705f61a37e91
- Packed model: b46a65befe47187f793ffb8108f74ab091338060060a54d23b69c385c3213d80

After that controller session, one UI-only firmware change corrected speech-export provenance: UNKNOWN_CAPTURE replaces an unjustified MICROPHONE label for arbitrary API jobs. B rebuilt successfully but is NOT FLASHED/HARDWARE-VERIFIED. Staged B SHA256: 2ff036e17b7f977692fe9604a7f3121afebbeca50ae7dad72b76a633f27f0673. The prior tested B image is retained privately at speech/.pio/verified_before_ui/firmware.bin. A/model sources remain unchanged. Deploy B before a fresh campaign; do not assign the staged hash to old observations.

## Work completed while boards are away

- Explicit offline UI preview: `python scripts/browser_bridge.py --preview-only`; loopback8765 `/simulation`. API always503 with knownfalse, no token injection or synthesized decisions. Browser movement/tour and sensor-disable controls inspected; screenshot `benchmarks/reports/figures/offline_house_preview.png`. This is not a live MCU demonstration.
- Preview is currently running in a background terminal process. Stop only that process before starting a serial bridge; do not kill unrelated Python processes. No serial process should remain running.
- Reports: `benchmarks/reports/PROJECT_VALIDATION_SUMMARY.md`, `PROJECT_COMPLETION_TABLE.csv`, `CURRENT_EVIDENCE.json`, hardware cases/trials/statistics and PNG/SVG figures. Regenerate curated summary with `python -m scripts.release_report` (explicit session selection, not latest-file guessing).
- Reconciled README, proposal comparison, architecture, RTOS design, memory, APIs, model/voice status, hardware status, requirements, literature and publication matrix. Original DOCX unchanged.
- Added `scripts/resume_validation.py`: requires COM5 and COM7; controller HIL, recovery, temporary USBbridge8766, synthetic audio; stops on failures. --deploy-staged deploys B with MAC verification and NVS preservation; no output enabling or phone changes. It does not certify human audio/physical hardware.

## When boards return: ordered continuation

1. Check both ports/identities and current firmware before changing anything; keep physical outputs inhibited. Do not reopen wiring questions for the virtual demo. Run `python scripts/resume_validation.py --deploy-staged`; retain failures and never auto-retry uncertain actuator operations as new commands.
2. Recovery needs a final clean original-policy read. USB re-enumeration may require waiting/reopening handles; current script has limited reconnect handling. If a transport exception occurs, record it, repair host reconnection and re-run. No power-cut/brownout claim from task-WDT.
3. Debug audio end-to-end. `scripts/audio_bench.py` uses `benchmarks/data/synthetic_speech/manifest.csv`:16 offline TTS phrases+silence/noise. All model inference must remain onB. Bridge can emit safe path/exception/byte-count diagnostics without logging tokens/audio. Confirm complete upload, queuedjob, phase progression, proper result and API responsiveness. Retain every mismatch; do not tune on this tiny corpus and claim unbiased accuracy.
4. Start serialbridgeCOM7 and open houseUI. EnterignoredlocalAPI token without printing/sharing it. Move person all4rooms; verify actual A states, lux/temp thresholds, expiry, sensorunknown, dwell/manualhold, exports. Then laptop microphoneuserbutton→Brecognition→explicitconfirm→Astate. Inspect media browser permission only for authorized local capture. No cloud/browser STT.
5. The staged UI fixes speech-export provenance: UNKNOWN_CAPTURE is used because the API cannot attest input origin. Label human/TTS recordings independently before research analysis. Audio benchmark separately labels SIMULATED inputs. The new image compiled but needs deployment/MCU retest. No fabricated accuracy.
6. Perform human/negative/noise data collection and matchedsingle/dual baselines only after functional speech works. Both boards' measured energy, long runs and independent evaluation remain required for those research claims.
7. Physical PIR/relay/driver testing is outside the user's current virtual scope. Do not enable pins from possession of parts alone. Confirmactualpins/supply/polarity/low-voltage load and review protection first. Mains remainsprohibited.

## Reproduction / tooling

```powershell
python -m pip install -r requirements-dev.txt
python scripts/fetch_speech_dependencies.py
python -m platformio run -d native -e native_voice
python -m platformio run -d speech -e speech_intelligence
python scripts/flash_native.py --role A
python scripts/flash_native.py --role B-speech
python -m pytest tests -q --junitxml=benchmarks/raw/host_tests.xml
python scripts/resume_validation.py --deploy-staged
python -m scripts.release_report
```

Build sequentially; PlatformIO shares bookkeeping. Esptool5.4 modern stub succeeded where bundled4.5.1 uploads failed. Flasher verifiesMAC; full modelflashoffset0x610000; `--app-only` assumesverifiedpartition/model/app0. NVS preserved; no eFuses/securitybootactivation. `native_intelligence` is an optional non-speech target; do not assume its binary still exists. Active B target is `speech_intelligence`.

Credentials are in ignored `local_credentials.json` and private header. Backups/binaries containsecrets; do not publish. No Git repository currently initialized. No subagents requested; do not delegate without authorization under current developer rules.

## Research honesty

No bestmodel, energybenefit, physical four-room localization, three-wayfeedback, UPS, MQTTdatabase, signedOTA, secureboot/encryption or splitneural speedup is proven. Literature includesMCUNet/MCUNetV2/MicroNets/MLPerfTiny,RockNet2025 and direct2026split-MCUpriorart; see `docs/LITERATURE_REVIEW.md`. The review is focused, not an exhaustive systematicreview. Proposed useful question: memory/deadline-aware semantic task allocation under concurrent inference/control/interference, measuredagainstfairbaselines. Implementedplatform alone is notpublicationnovelty.
