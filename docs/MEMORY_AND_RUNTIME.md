# Memory placement and resource limits

Each ESP32-S3 N16R8 has its own 16 MiB flash and 8 MiB PSRAM. Two boards do not provide a shared 16 MiB RAM arena. Current native clocks/cache/RTOS settings and static stacks are in [embedded design](EMBEDDED_SYSTEM_DESIGN.md).

## Placement

| Resource | Placement and bound | Reason / limitation |
|---|---|---|
| Safety/device state, control/link queues | Static internal SRAM; 10 registered devices; bounded queues | Predictable ownership and no per-action allocation |
| Control and LinkTask stacks | 6144 B static internal each | Stack free minima measured; not all SDK tasks are static |
| Diagnostics stack | 8192 B static internal per board | Console/JSON and low-priority NVS work |
| SpeechTask stack | 12288 B static internal | Model work isolated from A control |
| PCM clip | One aligned 128000 B PSRAM buffer | Up to four seconds 16 kHz mono PCM16; rejects overlapping jobs |
| Speech tensors/runtime | ESP-SR-owned allocations, PSRAM eligible | Not allocated from the legacy generic arena; actual initialization footprint matters |
| Medium libc allocations on B | PSRAM threshold 1024 B enabled before model init | Explicit internal/DMA-capability allocations retain their required region |
| UI and model | Flash; model image 3788231 B in 6 MiB partition | Packed flash size is not activation/RAM footprint |
| Legacy ModelMemory arena | Explicit PSRAM-only, <=2 MiB, 512 KiB planned PSRAM reserve, 64 KiB internal admission | Optional unused experiment; its reserve is not a guarantee for ESP-SR |
| Audit / measurement history | A audit 32 volatile commands; browser CSV <=30000 samples | Browser history is not board RAM or a persistent MCU database |

Speech initialization checks at least 80000 B free internal memory and 5 MiB free PSRAM before allocation. It then allocates the clip/model and reports readiness. This startup admission does not guarantee minimum headroom under all future HTTP/audio loads. No per-frame application heap calls are added; the library's complete allocation/peak behavior needs profiling. PCM is cleared after each finished job.

## Observations from the retained controller session

HARDWARE_MEASURED, bytes; model resident on B, no audio inference workload during command timing:

| Board | Lowest sampled internal free | SDK low-water since boot | Lowest largest internal block | Lowest sampled PSRAM free | Lowest largest PSRAM block |
|---|---:|---:|---:|---:|---:|
| A | 190075 | 180851 | 151552 | 8386111 | 8257536 |
| B | 104655 | 55459 | 59392 | 4089807 | 3866624 |

Low-water since boot includes initialization/earlier work; sampled free is a periodic reading. B did **not** maintain a 64 KiB internal low-water reserve. Total free bytes do not prove that a contiguous allocation fits. The current model fits at initialization, but completed peak audio/concurrency/fragmentation evidence is pending. Do not increase model size just to consume all spare PSRAM.

## Flash map

| Partition | Offset | Size |
|---|---:|---:|
| NVS | 0x9000 | 0x5000 |
| OTA metadata | 0xe000 | 0x2000 |
| app0 | 0x10000 | 0x300000 |
| app1 | 0x310000 | 0x300000 |
| model | 0x610000 | 0x600000 |
| storage | 0xc10000 | 0x3f0000 |

Native A and speech B use the same map. Dual slots and startup image confirmation are not a completed OTA updater. Flash commits can stall caches even from low-priority Diagnostics. NVS errors never trigger an automatic erase, because that would destroy policy/replay history.

## Verification and next optimization

Host tests compile the actual arena, wire, policy and safety logic. Hardware probes verify initialized PSRAM and heap integrity. Next measure simultaneous PCM upload, inference, radio retries and dashboard streaming; log internal free/min/largest, PSRAM largest, all task stacks, queue drops, job rejection and reset causes. Add bounded overload tests before changing buffers/cache thresholds. Compare measured quality/latency/energy under identical inputs; RAM utilization alone is not a performance metric.

[Current raw evidence and tables](../benchmarks/reports/PROJECT_VALIDATION_SUMMARY.md).
