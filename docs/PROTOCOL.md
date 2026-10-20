# Active authenticated wire protocol

Updated 20 October 2026. All multibyte integers are big-endian. The active transport uses a pinned encrypted unicast peer and a mandatory 16-byte truncated SHA256-HMAC over the complete serialized envelope/payload. The total application frame is at most 238 bytes; authentication leaves at most 204 payload bytes. CRC detects accidental corruption and provides no authentication by itself.

| Offset | Bytes | Envelope field |
|---|---|---|
| 0 | 2 | Magic 0x4347 |
| 2 | 1 | Version 2 |
| 3 | 1 | Message type |
| 4 | 1 | Flags: ACK_REQUIRED=1, RETRY=2, URGENT=4 |
| 5 | 1 | Reserved zero |
| 6 | 2 | Source node |
| 8 | 2 | Destination node |
| 10 | 4 | Sequence |
| 14 | 2 | Payload length |
| 16 | 2 | CRC16-CCITT over bytes 0..15 followed by payload |
| 18 | variable | Payload |
| 18+payload length | 16 | Mandatory application authentication tag |

Unauthenticated/truncated/bad-tag/wrong-peer frames are rejected before dispatch. Queue storage is fixed at eight frames; each normal loop handles at most four. Send API success means enqueued, not delivered or executed.

## Room telemetry (A=1 â†’ B=2, type 4)

| Payload offset | Bytes | Field |
|---|---|---|
| 0 | 1 | Schema 2 |
| 1 | 1 | Bedroom zone ID 1 |
| 2 | 1 | Motion boolean |
| 3 | 1 | Occupancy: unknown=0, occupied=1, likely=2, vacant=3 |
| 4 / 5 | 1 each | Actual kernel light / fan boolean |
| 6 | 2 | Reserved zero |
| 8 / 12 | 4 each | Persistent actuator boot identity / uptime milliseconds |
| 16 | 4 | Last processed command ID |
| 20 | 1 | Mode: home=0, away=1 |
| 21 / 22 | 1 each | Light/fan auto flags / manual-hold flags |
| 23 | 1 | Reserved zero |
| 24 | 4 | Vacancy grace milliseconds |
| 28 / 32 | 4 each | Remaining light/fan timer milliseconds; 0 inactive, 1 overdue pending |
| 36 / 40 | 4 each | Software light/fan on-time milliseconds since boot |

Reports are 44 payload bytes (78 authenticated frame bytes) and normally sent once per second. B rejects old/duplicate sequence numbers within one boot nonce. A newer boot identity resets the stream. Legacy 16-byte commands/32-byte ACKs are rejected by exact payload lengths even though the outer envelope remains version 2; update both boards together. After five seconds without current state, controls are refused and displayed output state becomes unknown.

## Command (B=2 â†’ A=1, type 5)

24-byte payload: persistent actuator boot identity uint32, request ID uint32, operation uint8, device uint8, action uint8, reserved zero uint8, value uint32, issued-at A uptime uint32, TTL milliseconds uint32. TTL decode permits 100..5000 ms; B uses 3000 ms. A checks signed age with 100 ms future-skew allowance when executing. See [feature/API guide](HOME_AUTOMATION_FEATURES.md) for operation semantics.

A accepts only the next nonzero ID for its current boot. An identical last command returns its cached ACK without re-execution. A different payload using the same ID is rejected and must not replace the cached outcome. ID exhaustion requires a new actuator boot; IDs do not wrap. Native A commits a monotonic 32-bit NVS boot counter before enabling control; counter exhaustion and NVS errors fail closed. Preserving NVS is required. Factory erase, old-NVS snapshot restoration and key lifecycle remain separate replay/security problems. B rejects a lower boot identity during its current session, but has no persistent highest-peer-boot record.

## Execution ACK (A â†’ B, type 6)

40-byte payload: persistent actuator boot identity uint32, request ID uint32, status uint8, actual light uint8, actual fan uint8, mode uint8, execution microseconds uint32, followed by the exact original 24 command bytes. B checks the echoed command as well as ID/boot, binding the response to the proposed operation.

Status: applied=0, denied=1, invalid=2, replay=3, wrong boot=4, self-test disabled=5, expired=6. Scenes may partially apply if one kernel request is denied. An ACK captures execution-time state; periodic telemetry supplies current state. B permits one pending transaction and retries at 300 ms intervals for four total attempts. Timeout leaves the outcome unknown.

PMK, LMK, HMAC key, AP password and control token are generated locally. Keys are embedded in the prototype image. The protocol is not a substitute for secure provisioning, secure boot, flash encryption or a full threat assessment.

Tensor message declarations are reserved interfaces, not implemented distributed inference.

## Virtual-house inputs and state

All records use the same authenticated transport and explicit big-endian encoding. Synthetic inputs never enter the real bedroom PIR tracker.

| Message | Payload layout | Validation / meaning |
|---|---|---|
| SENSOR_EVENT B→A | 36 B: ASCII SIM, schema 1; A boot uint32; input sequence uint32; A-issued uptime uint32; four entries of flags uint8, temperature_deciC uint16, lux uint16 | Sequence nonzero/newer; boot match; age -100..3000 ms; flags <=63; temperature <=600; lux <=2000 |
| NODE_STATUS A→B | 32 B: ASCII SSM, schema 1; boot, input sequence, A uptime, policy processing µs (uint32 each); output mask uint8, known-room mask uint8, occupied-room mask uint8, physical-output-enable uint8; accepted/rejected uint32 each | 8 virtual output bits; room masks <=15; enable boolean. Output state is software, not electrical feedback. |

Inputs are latest-mailbox data. A expires its received-input lease after three seconds without accepted input. Virtual state reports are normally sent at 10 Hz. Missing telemetry is unknown, never replaced with host-generated decisions. Operation 6 addresses the eight virtual devices and still passes SafetyKernel. All current physical outputs are inhibited.
