# Memory and runtime engineering

The two boards each own their RAM. The application exchanges compact messages;
it does not create a shared 16 MiB heap. This implementation prepares predictable
memory use for a future ESP-SR audio pipeline on A and a small predictor on B.
No trained model is deployed by these changes.

## Memory placement

| Memory | What belongs here | Current implementation |
|---|---|---|
| Internal SRAM | Safety state, task stacks, DMA-capable audio buffers, short queues, hot scratch data | Static device table and eight-frame RX queue per board; dedicated HTTP task on B |
| PSRAM | Larger model tensors, long audio/history buffers when supported by their drivers/runtime | Explicit `MALLOC_CAP_SPIRAM` model arena; no implicit internal-RAM fallback |
| Flash | Firmware, immutable UI, model assets | Dashboard served directly from a static flash string; model partition reserved |

PSRAM is capacity, not a universal performance accelerator. Keep the hot
working set small and measure each model's actual operators. Future audio DMA
buffers should use the required internal/DMA capabilities for this pinned SDK;
do not move them to PSRAM merely because external memory is available.

`ModelMemory::reserve()` allocates once, rejects requests above 2 MiB, leaves a
planned 512 KiB PSRAM reserve, and requires at least 64 KiB free internal memory.
These are **engineering limits, not measured optimal values**. They can be
changed after workload profiling. It checks the largest free PSRAM block as
well as total free space. The allocator never steals internal SRAM when PSRAM
allocation fails. The aligned arena uses checked arithmetic, tracks peak use
and rejected allocations, and performs no heap calls between `allocate()` and
`reset()`. Reset only after all tensor users have finished; reserve/release and
arena use have one owning task. The current firmware leaves this arena
unallocated because there is no model yet.

## Flash layout

The custom `clawgrid_s3_n16r8` board selects QIO flash and OPI PSRAM. Both modules
were identified on COM5 and COM7 as 16 MB flash with 8 MB embedded PSRAM. Runtime
`memory_config_ok` also checks the initialized PSRAM size.
This check uses `esp_spiram_get_size()`: Arduino 2.0.14's `getPsramSize()`
returns heap-accounted bytes, which exclude allocator overhead and cannot be
compared for exact equality with the physical 8 MiB capacity. The first board
capture exposed this distinction; its failed probe is retained as diagnostic evidence.

`partitions_n16r8.csv` uses the 16 MiB address space:

| Partition | Offset | Capacity |
|---|---:|---:|
| NVS | 0x9000 | 20 KiB |
| OTA metadata | 0xe000 | 8 KiB |
| Application 0 | 0x10000 | 3 MiB |
| Application 1 | 0x310000 | 3 MiB |
| Future model data | 0x610000 | 6 MiB |
| Future storage | 0xc10000 | 4032 KiB |

The first 36 KiB hold bootloader/partition-table space. The two application
slots reserve room for future OTA; an OTA update service is not implemented.
The model and storage partitions are reserved, not loaded with fabricated
models or mounted as a filesystem. Partition changes require reflashing the
partition table and validating existing data before a deployment with user data.

## Scheduling and bounded work

- Node A's static ControlTask owns SafetyKernel and GPIO. PIR input is debounced over
  200 ms, vacancy timing handles uint32 timer wrap, and deferred OFF is retried.
  State reports contain actual SafetyKernel outputs, not parser guesses.
- ESP-NOW callbacks only validate input length and copy into a static queue.
  LinkTask drains at most four frames per five-millisecond target cycle; overflow increments a counter.
- Node B processes reports in its static LinkTask and serves HTTP in a separate task with
  a 6144-byte stack, at most two client sockets, and one-second send/receive
  timeouts. A critical section protects only the short shared state copy.
- Application HTTP storage is fixed: 1536-byte response buffer and 128-byte
  command buffer. The SDK bounds URI and header sizes at 512 and 1024 bytes in
  the installed configuration. Oversized command bodies are rejected before
  reading into application storage. The HTTP/lwIP SDK still allocates its own
  bounded connection structures: this is not a claim of zero system-wide heap use.
- Report payloads are 44 bytes, the envelope is 18 bytes and the authentication
  tag is 16 bytes. Periodic full state
  repairs loss of individual reports. State older than five seconds becomes
  unknown. Sequence checks discard duplicates within a boot session; they do
  not by themselves authenticate senders. The active transport now adds paired
  encrypted unicast, pinned source MACs and mandatory HMAC verification.
  Authenticated control proposals still pass through SafetyKernel, with one
  pending transaction, cached duplicate outcomes and bounded ACK retry.

The explicit project tasks each reserve a 6144-byte internal stack and static
task control block. A has ControlTask and LinkTask; B has LinkTask plus the
SDK-managed HTTP and Arduino diagnostics tasks. A's command and ACK queues
have eight entries each; its latest-state mailbox has depth one. Both boards
have a 16-line fixed diagnostic queue. These allocations trade some internal
RAM for explicit ownership and scheduling. Current measurements, not the
pre-RTOS memory baseline, determine the remaining headroom.

## Reproducible verification

```
python -m pytest tests/ -q
python -m platformio run -e node_voice -e node_intelligence
python -m platformio run -e node_voice -t upload --upload-port COM5
python -m platformio run -e node_intelligence -t upload --upload-port COM7
python scripts/capture_hardware.py --ports COM5 COM7 --seconds 30 --probe
```

Only flash with the connected hardware and low-voltage output polarity verified.
The current bench has bare boards and no loads. UART command `t` runs protocol,
arena, timer-wrap and parser checks (parser on B); command `m` temporarily
allocates, writes, reads, overflow-checks and releases a 256 KiB PSRAM arena.
It reports `HARDWARE_MEASURED` diagnostic time, not neural inference performance.
SafetyKernel's no-GPIO tests run only at boot, before real devices are registered.

The capture script saves raw logs, JSON samples and firmware hashes. Record
internal free/minimum/largest block, external free/largest block, stack minimum
free bytes, queue drops and transmit enqueue failures. Largest-block loss with
stable total free memory can expose fragmentation. Serial logs sample the loop
task; `/api/memory` samples the HTTP task. A short idle capture does not prove
long-term stability or audio/model performance. The next gate is a sustained
HTTP + radio + audio test with actual model allocation and load.

## Evidence and sources

Implementation was checked against the installed Arduino-ESP32 2.0.14 / ESP-IDF
SDK headers and sources (`esp_heap_caps.h`, `portmacro.h`, `esp_http_server.h`,
`sdkconfig.h`). The stack element type in this Xtensa port is one byte, so its
high-water value is reported as bytes. The memory allocator, external RAM and
ESP-NOW references were also retrieved and checked on 20 October 2026:

- [ESP-IDF heap capability allocator](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32s3/api-reference/system/mem_alloc.html)
- [External RAM restrictions](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32s3/api-guides/external-ram.html)
- [ESP-NOW callback and delivery semantics](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32s3/api-reference/network/esp_now.html)
- [HTTP server configuration](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32s3/api-reference/protocols/esp_http_server.html)
- [ESP-SR model selection and partitioning](https://docs.espressif.com/projects/esp-sr/en/latest/esp32s3/flash_model/README.html)

For AI, first integrate the supported ESP-SR audio path after a microphone is
available. Keep speech model weights in their flash partition when the runtime
supports it, allocate audio/model buffers at startup, and reject model startup
if headroom is insufficient. Compare a small quantized temporal predictor with
fixed and statistical timers only after collecting independently labeled data.
Move a model between boards only after measuring compute, transfer and accuracy
on identical workloads; task parallelism is already useful without tensor splitting.
