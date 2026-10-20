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

## Room telemetry (A=1 → B=2, type 4)

| Payload offset | Bytes | Field |
|---|---|---|
| 0 | 1 | Schema 2 |
| 1 | 1 | Bedroom zone ID 1 |
| 2 | 1 | Motion boolean |
| 3 | 1 | Occupancy: unknown=0, occupied=1, likely=2, vacant=3 |
| 4 / 5 | 1 each | Actual kernel light / fan boolean |
| 6 | 2 | Reserved zero |
| 8 / 12 | 4 each | Actuator boot nonce / uptime milliseconds |
| 16 | 4 | Last processed command ID |
| 20 | 1 | Mode: home=0, away=1 |
| 21 / 22 | 1 each | Light/fan auto flags / manual-hold flags |
| 23 | 1 | Reserved zero |
| 24 | 4 | Vacancy grace milliseconds |
| 28 / 32 | 4 each | Remaining light/fan timer milliseconds; 0 inactive, 1 overdue pending |
| 36 / 40 | 4 each | Software light/fan on-time milliseconds since boot |

Reports are 44 payload bytes (78 authenticated frame bytes) and normally sent once per second. B rejects old/duplicate sequence numbers within one boot nonce. A boot change resets the stream. After five seconds without current state, controls are refused and displayed output state becomes unknown.

## Command (B=2 → A=1, type 5)

16-byte payload: actuator boot nonce uint32, request ID uint32, operation uint8, device uint8, action uint8, reserved zero uint8, value uint32. See [feature/API guide](HOME_AUTOMATION_FEATURES.md) for operation semantics.

A accepts only the next nonzero ID for its current boot. An identical last command returns its cached ACK without re-execution. A different payload using the same ID is rejected and must not replace the cached outcome. ID exhaustion requires a new actuator boot; IDs do not wrap. The boot nonce is a random 32-bit value, not a guaranteed persistent unique counter; high-assurance replay resistance across many power cycles remains a production hardening issue.

## Execution ACK (A → B, type 6)

32-byte payload: actuator boot nonce uint32, request ID uint32, status uint8, actual light uint8, actual fan uint8, mode uint8, execution microseconds uint32, followed by the exact original 16 command bytes. B checks the echoed command as well as ID/boot, binding the response to the proposed operation.

Status: applied=0, denied=1, invalid=2, replay=3, wrong boot=4, self-test disabled=5. Scenes may partially apply if one kernel request is denied. An ACK captures execution-time state; periodic telemetry supplies current state. B permits one pending transaction and retries at 300 ms intervals for four total attempts. Timeout leaves the outcome unknown.

PMK, LMK, HMAC key, AP password and control token are generated locally. Keys are embedded in the prototype image. The protocol is not a substitute for secure provisioning, secure boot, flash encryption or a full threat assessment.

Tensor message declarations are reserved interfaces, not implemented distributed inference.
