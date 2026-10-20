# Hardware status

Updated 2026-10-20. User confirmed the test setup contains boards only, with no relays, loads or sensors attached, and authorized identity reads, uploads and memory testing.

Latest RTOS validation: [70 passing hardware checks and 1,000-command run](../benchmarks/reports/HARDWARE_TEST_RESULTS.md). Host suite: 40 passing tests. The RTOS run spanned approximately 209 seconds including a 60-second soak; this is not long-duration stability evidence. The earlier controller baseline is retained separately.

## HARDWARE_MEASURED identities

| Board | Port | MAC | Chip | Flash | Embedded PSRAM |
|---|---|---|---|---|---|
| A | COM5 | e0:72:a1:f6:a9:68 | ESP32-S3 v0.2 | 16 MiB | 8 MiB |
| B | COM7 | e0:72:a1:f5:bc:b0 | ESP32-S3 v0.2 | 16 MiB | 8 MiB |

Both expose USB Serial/JTAG and have 40 MHz crystals. Identity was read with esptool. Complete 16 MiB pre-upgrade flash backups are retained locally in `benchmarks/backups/`. Runtime memory availability is separate from chip capacity; see the capture artifacts and memory report.

## Remaining hardware work

- Microphone capture and on-board speech models are not implemented or tested. Select and connect a compatible microphone before acoustic evaluation.
- PIR, relay polarity and actual light/fan operation have not been verified in this bare-board test. Use low-voltage DC loads only. A single PIR provides evidence for its assigned zone only.
- Audio playback is not implemented or tested.
- The phone needs only a standard browser. The embedded interface accepts text proposals; it is not offline speech recognition.
- No split model or occupancy predictor is deployed. The boards do not share one address space or pool PSRAM.
