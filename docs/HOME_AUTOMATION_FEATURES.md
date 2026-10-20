# Home automation features and API

Current native build: real-bedroom software controller plus four virtual rooms; physical GPIOs inhibited. Nothing is wired. B is available on COM7 and has a live USB bridge on localhost8766. A remains unavailable; port8765 is still the explicit offline preview. See [simulation lab](SIMULATION_LAB.md) for startup/build instructions.

## Features and boundaries

| Feature | Implemented behavior | Boundary |
|---|---|---|
| Four-room house | Person movement/tour; bedroom/living/kitchen/study; light/fan each | Browser position/sensors SIMULATED; A must decide output states |
| Environment | Per-room PIR/presence availability, door, temperature and lux | Virtual only; door alone cannot establish occupancy |
| Virtual policy | Occupied + lux <300 => light; occupied + temperature >=28 C => fan | Deterministic threshold policy on A, five-second virtual vacancy grace |
| Dropout | Three-second input lease, unknown evidence and stale telemetry | Manual hold is not canceled by missing automatic evidence |
| Device safety | Three-second dwell, five-minute manual hold, four-hour run limit, whitelist | Every device request passes SafetyKernel |
| Bedroom controls | ON/OFF/toggle; resume auto; Home/Away/Sleep/All-off | Two software outputs; no connected loads; scene can partially apply on denial |
| Bedroom timers | Relative auto-off per device, cancel/replace, 1 s–24 h | Transient; no ON output/timer restored after reboot |
| PIR policy | Separate assigned-bedroom tracker, configurable 30–1800 s grace | Real PIR unwired; no multi-room localization claim |
| Persistence | CRC/versioned auto-policy/grace/Away record and boot counter | Flash writes coalesced; timers/holds/audit are volatile |
| Assistant | Deterministic text preview; B's learned MultiNet6 speech proposal + explicit confirmation | Speech model ready; synthetic command recognition currently fails; live paired workflow not validated |
| Measurement | Board timing, queues, heap/stack and state; browser raw/summary exports | Source labels separate; software state is not physical feedback |

No dimming, fan speed, calendar schedules, actual air-quality/smoke/leak sensor, real energy meter, cooperative shedding or UPS is implemented. They require appropriate hardware and tested policies.

## Local browser

- Offline interface: `python scripts/browser_bridge.py --preview-only`; all API calls return 503.
- Returned boards: `python scripts/browser_bridge.py --serial COM7`; open localhost port 8765 `/simulation`, enter the API token from the ignored local credentials and start inputs.
- Direct board AP: ClawGrid-Assistant, `http://192.168.4.1/simulation`. Laptop localhost enables secure-context microphone capture; plain HTTP phone access is for visual/control use.

The token stays in tab memory. The host bridge forwards supplied credentials/bytes, and performs no automatic token injection or MCU inference fallback. Do not run another serial owner concurrently.

## Control API

`POST /api/control`, header `X-ClawGrid-Token`, body `op device action value` in ASCII. HTTP 202 means queued, not executed. Acknowledged outcomes are in authenticated `GET /api/transaction`: phase 1 pending, 2 ACK, 3 timeout. One pending transaction; busy/stale state returns 409.

| Op | Purpose | Device | Action | Value |
|---|---|---|---|---|
| 1 | Bedroom device | 0 light / 1 fan | 0 OFF / 1 ON / 2 toggle | 0 |
| 2 | Bedroom scene | 0 | 0 Home / 1 Away / 2 Sleep / 3 All-off | 0 |
| 3 | Bedroom timer | 0 light / 1 fan | 0 | delay ms or 0 cancel |
| 4 | Auto policy | 0 light / 1 fan | 0 disable / 1 enable | 0 |
| 4 | Vacancy grace | 2 | 0 | 30000–1800000 ms |
| 5 | Resume bedroom auto | 0 light / 1 fan | 0 | 0 |
| 6 | Virtual device | 0..7: room index*2 + light=0/fan=1 | 0 OFF / 1 ON / 2 toggle | 0 |

ACK status: 0 applied, 1 denied, 2 invalid, 3 replay, 4 wrong boot, 5 self-test disabled, 6 expired. ACK reflects execution-time software state; current virtual outputs come from `/api/simulation`.

## Other APIs

| Endpoint | Function |
|---|---|
| GET /api/state | Bedroom state/age; unknown after stale report |
| GET /api/memory | B heap/PSRAM/largest blocks/stack diagnostics |
| GET /api/rtos | B LinkTask progress, observed maxima/stack/watchdog |
| POST /api/voice | Bounded deterministic text proposal only |
| GET /api/simulation | Authenticated A virtual-state snapshot; input_source SIMULATED, processing_source HARDWARE_MEASURED |
| POST /api/simulation/input | Token required; 12 integers: flags temperature_deciC lux for each of four rooms |
| GET /api/speech | Token required; model phase/job/proposal/metrics; never actuation authorization |
| POST /api/speech/audio | Token required; mono 16 kHz PCM16LE, 3200..128000 bytes; one bounded job |

Sensor flag bits: 0 PIR motion, 1 presence, 2 door open; 3 PIR available, 4 presence available, 5 door available. Temperature range 0..600 deci-C, lux 0..2000. B adds the current A boot/uptime/sequence; A validates identity/order/age. Sensor input acceptance is not proof of output actuation.

Speech phases: 0 initializing, 1 idle, 2 receiving, 3 processing, 4 complete, 5 fault. Inference time includes scheduled wall time across frames, not host transport/capture or electrical actuation. Unknown command ID 0 never authorizes output.

## Deep-review extension

Authenticated GET `/api/evidence` executes 23 shadow monitor cases on B. Its response explicitly labels synthetic inputs and MCU execution, reports a pass mask and object bytes, and states physical_verification=false. This is a diagnostic component; it does not drive or protect physical GPIOs yet. See [the monitor specification](EVIDENCE_MONITOR_SPEC.md).

GET `/api/speech` now includes PCM CRC32 and bounded diagnostic_text. The benchmark compares the CRC before evaluating results. Decoder text is never actuation authorization. Speech exports use UNKNOWN_CAPTURE because the API does not attest microphone versus externally submitted TTS origin.
