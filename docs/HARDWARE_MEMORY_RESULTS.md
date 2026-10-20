# Bare-board memory and communication results

This is the earlier memory-upgrade baseline. The later controller firmware and its 1,000-command run are documented in [current hardware test results](../benchmarks/reports/HARDWARE_TEST_RESULTS.md).

Date: 2026-10-20. Source: **HARDWARE_MEASURED** unless explicitly stated otherwise.

## Setup and provenance

Two ESP32-S3 N16R8 boards, COM5 (A) and COM7 (B), powered by USB. No microphone, PIR, relays or loads attached. Both original 16 MiB flash images were backed up before uploads. Final uploads completed with esptool hash verification.

Command: `python scripts/capture_hardware.py --ports COM5 COM7 --seconds 30 --probe`

Raw evidence: [JSON](../benchmarks/raw/hardware/20261020T004141Z.json), [serial log](../benchmarks/raw/hardware/20261020T004141Z.log). The capture contains six periodic memory samples per board plus diagnostic and counter records. Firmware hashes are stored in the JSON.

## Results

| Quantity (bytes unless noted) | Node A | Node B |
|---|---:|---:|
| Physical flash | 16,777,216 | 16,777,216 |
| Initialized physical PSRAM | 8,388,608 | 8,388,608 |
| Minimum sampled internal free | 290,940 | 280,408 |
| Lowest reported internal heap low-water value | 289,744 | 280,028 |
| Minimum sampled largest internal block | 278,516 | 270,324 |
| Sampled PSRAM free | 8,375,219 | 8,373,719 |
| Sampled largest PSRAM block | 8,257,524 | 8,257,524 |
| Loop task stack minimum free | 5,932 | 5,820 |
| Temporary arena tested | 262,144 | 262,144 |
| Arena write/read and overflow rejection | PASS | PASS |
| Heap integrity after probe | PASS | PASS |
| Runtime self-test | PASS | PASS |
| Sampled RX queue drops / TX enqueue failures | 0 / 0 | 0 / 0 |

Node B's accepted report counter increased from 9 to 34 across its six samples, with invalid and replay counters zero. This demonstrates communication during the observation interval; it is not a packet-loss-rate experiment or authentication test. A's runtime result includes its boot safety-test status; B additionally exercises the strict text parser.

The one-shot arena diagnostic took 21,337 microseconds on A and 21,335 on B, including its memory checks. These are diagnostic durations, **not inference latency, memory bandwidth or statistical performance estimates**.

## Defect found and corrected

The first capture, [20261020T003930Z](../benchmarks/raw/hardware/20261020T003930Z.json), correctly recorded failed probes. Arduino 2.0.14's `getPsramSize()` sums heap-accounted bytes and excludes allocator overhead. Comparing that number to exact physical capacity incorrectly blocked allocation. The implementation now uses `esp_spiram_get_size()` for hardware identity and capability-based free/largest-block APIs for allocation decisions. The failed capture is retained, not replaced.

The capture utility now returns failure if required memory/self-test/probe records are missing or indicate failure, while preserving raw evidence.

## HOST_MEASURED verification

- Python suite: 36 passed.
- Both final firmware targets compiled and uploaded successfully.
- Static RAM: A 46,248 bytes; B 46,164 bytes. These build figures exclude runtime allocations.
- Application flash usage: A 688,921 bytes; B 707,225 bytes, each within its 3 MiB application slot.

## What remains unverified

This was a short bare-board test with one allocator probe per board. It does not establish long-term leak freedom, HTTP load behavior, physical actuation, audio quality, model fit, inference accuracy, energy savings or distributed speedup. Loop stack measurements do not measure the HTTP task stack. No neural model is deployed; the model arena is normally unallocated. The next experiment needs actual microphone/audio/model workloads plus concurrent network and HTTP traffic.

See [memory design](MEMORY_AND_RUNTIME.md), [model evaluation plan](MODEL_EVALUATION.md) and [literature/research direction](RESEARCH_DIRECTION.md).
