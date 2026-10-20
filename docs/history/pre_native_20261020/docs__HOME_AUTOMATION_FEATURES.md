# Home automation capabilities and use

Updated 20 October 2026. The active implementation controls one bedroom with two low-voltage output channels. The current bench has bare boards only. The following firmware features are implemented; physical-load validation is a separate test gate.

| Feature | Behavior | Evidence / boundary |
|---|---|---|
| Bedroom light and fan | Explicit ON/OFF or toggle through SafetyKernel | Application ACK reports accepted/denied outcome and actual software state |
| Motion lighting | Debounced PIR drives a local pending ON; vacancy policy retries OFF after a hold/dwell expires | Node A continues independently of B |
| Optional motion fan | Explicitly enabled fan policy follows the bedroom PIR | Disabled by default; no temperature measurement |
| Occupancy display | UNKNOWN / OCCUPIED / LIKELY_OCCUPIED / VACANT policy states | PIR evidence is not ground-truth occupancy |
| Home scene | Resumes automatic policies and clears manual holds/timers | Does not force either output ON |
| Away scene | Requests both OFF and pauses automatic starts after both requests succeed | May partially apply if one safety request is denied |
| Sleep scene | Requests light OFF and fan ON | Uses five-minute manual holds; later policy may apply |
| All-off scene | Requests both OFF and cancels timers after successful scene | Dwell applies; this is not an emergency-stop mechanism |
| Auto-off timers | Independent light/fan relative timers; 1 second–24 hours; replace/cancel | At expiry, OFF retries if dwell prevents it |
| Manual override | Manual device commands hold automation for five minutes | Maximum-run cutoff still applies |
| Resume automatic control | Clears a selected device's manual hold | Dwell remains enforced |
| Vacancy grace | User selects 30–1,800 seconds; default 120 | Current setting is volatile |
| Output protection | Three-second dwell, four-hour continuous maximum run, explicit whitelist | Hardware polarity/boot waveforms need real-load validation |
| Authenticated radio | Paired CCMP unicast, pinned source MAC, mandatory truncated SHA256-HMAC | Separate generated PMK, LMK and HMAC key per pair |
| Replay control | Actuator boot nonce plus ordered request IDs; exact duplicate returns cached ACK | No action re-execution on a retry; no production security certification |
| Acknowledgements | B retries up to four attempts, 300 ms apart | Timeout reports unknown outcome, never success |
| Local dashboard | Actual bedroom state, timers, manual flags, runtime memory and controls | Standard browser; two AP clients maximum |
| Protected controls | WPA2 AP password plus X-ClawGrid-Token on control and transaction APIs | HTTP inside the local protected network; no HTTPS |
| Text assistant | Explicit room/device/action preview and user confirmation | Current parser is deterministic; no neural speech model |
| Stale telemetry | After five seconds, state is unknown and new controls are rejected | Periodic full reports recover after link restoration |
| Audit | Last 32 processed command outcomes retained in a fixed ring on A | Volatile; retrieve over USB with `a` |
| Runtime counters | Memory regions/largest blocks/stack minima, drops, invalid/replay frames, TX enqueue failures | Queue overflow is bounded and counted |
| Software on-time | Since-boot light/fan on-time estimates | Not electrical energy; uint32 counters wrap after ~49.7 days |
| Diagnostic tests | Boot checks, PSRAM probe, HTTP loopback, authentication/replay/ACK/outage injection | Explicit USB bench commands; raw test evidence retained |
| Explicit RTOS control | Periodic ControlTask separated from LinkTask and diagnostics | Static stacks/queues, readiness events, watchdog registration and task timing/stack telemetry |

## Browser setup

1. Run `python scripts/provision_local.py` before compiling a fresh pair. It creates local credentials once and does not silently rotate an existing pair.
2. Upload A and B sequentially. Read the AP password and API token from the ignored `local_credentials.json` on your own computer.
3. Join **ClawGrid-Assistant** with its generated password. Open `http://192.168.4.1` in a standard browser.
4. Enter the API token in the password field. It is kept only in that tab's memory. Submit controls and inspect the acknowledged result. The bedroom display refreshes from telemetry.

Do not publish the local credentials file or the private build header. Firmware embeds the bench keys and is not protected against physical extraction. Reprovision different board pairs with different credentials. No phone modification, application installation or cloud account is needed.

## Control API

`POST /api/control` uses plain ASCII `op device action value`, with the `X-ClawGrid-Token` header. A 202 response means queued, not applied. Poll authenticated `GET /api/transaction`: phase 1=pending, 2=acknowledged, 3=timeout. Status 0=applied, 1=denied, 2=invalid, 3=replay, 4=wrong boot, 5=self-test disabled. The ACK reflects the command's execution outcome; the periodic state endpoint reflects current state.

| Op | Meaning | Device | Action | Value |
|---|---|---|---|---|
| 1 | Device | 0 light / 1 fan | 0 OFF / 1 ON / 2 toggle | 0 |
| 2 | Scene | 0 | 0 Home / 1 Away / 2 Sleep / 3 All-off | 0 |
| 3 | Timer | 0 light / 1 fan | 0 | delay milliseconds, or 0 cancel |
| 4 | Auto policy | 0 light / 1 fan | 0 disable / 1 enable | 0 |
| 4 | Vacancy grace | 2 | 0 | 30,000–1,800,000 milliseconds |
| 5 | Resume | 0 light / 1 fan | 0 | 0 |

Read-only endpoints: `/api/state`, `/api/memory`; `/api/voice` provides a bounded text preview. The firmware accepts one pending control at a time, giving HTTP 409 if busy or stale. There are no schedule queues that grow with use.

`GET /api/rtos` exposes Node B's measured LinkTask jobs, timing maxima, stack margin and watchdog subscription. These are engineering diagnostics; an observed maximum is not a proven worst-case execution-time bound.

## Reboot and sensing boundaries

Outputs initialize OFF. Timers, holds, policies, scenes and audit entries reset on reboot. Default motion lighting can subsequently turn the light ON when valid PIR motion is detected. Only the bedroom is instrumented in the intended prototype; the present bare-board tests have no PIR attached.

Clock/calendar schedules, dimming, fan speed, temperature/air-quality, door/window, smoke/leak sensing and multi-room automation require the appropriate hardware and separate validated policies. Voice requires a microphone and ESP-SR integration. Do not advertise these as working capabilities.
