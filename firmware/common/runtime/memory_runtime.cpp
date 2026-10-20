#include "memory_runtime.h"
#include <esp_heap_caps.h>
#include <esp32s3/spiram.h>

static constexpr uint32_t kInternal = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
static constexpr uint32_t kExternal = MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT;
static constexpr size_t kInternalReserve = 64 * 1024;
static constexpr size_t kExternalReserve = 512 * 1024;

MemorySnapshot memory_snapshot() {
    MemorySnapshot s = {};
    s.internal_free = heap_caps_get_free_size(kInternal);
    s.internal_min = heap_caps_get_minimum_free_size(kInternal);
    s.internal_largest = heap_caps_get_largest_free_block(kInternal);
    // Arduino getPsramSize() sums heap-accounted bytes, excluding overhead.
    s.psram_total = psramFound() ? esp_spiram_get_size() : 0;
    s.psram_free = heap_caps_get_free_size(kExternal);
    s.psram_largest = heap_caps_get_largest_free_block(kExternal);
    s.stack_free_bytes = uxTaskGetStackHighWaterMark(nullptr); // ESP-IDF reports bytes
    return s;
}
bool memory_hardware_matches() {
    return ESP.getFlashChipSize() == 16u * 1024 * 1024 &&
           psramFound() && esp_spiram_get_size() == 8u * 1024 * 1024;
}
void memory_log(const char *node, uint32_t rx_dropped, uint32_t tx_failed) {
    if (!Serial) return;
    MemorySnapshot s = memory_snapshot();
    Serial.printf("{\"source\":\"HARDWARE_MEASURED\",\"kind\":\"memory\",\"node\":\"%s\","
                  "\"uptime_ms\":%lu,\"flash_bytes\":%u,\"internal_free\":%u,"
                  "\"internal_min\":%u,\"internal_largest\":%u,\"psram_total\":%u,"
                  "\"psram_free\":%u,\"psram_largest\":%u,\"stack_free_bytes\":%u,"
                  "\"rx_dropped\":%u,\"tx_failed\":%u,\"memory_config_ok\":%s}\n",
                  node, millis(), ESP.getFlashChipSize(), s.internal_free, s.internal_min,
                  s.internal_largest, s.psram_total, s.psram_free, s.psram_largest,
                  s.stack_free_bytes, rx_dropped, tx_failed,
                  memory_hardware_matches() ? "true" : "false");
}
bool ModelMemory::reserve(size_t bytes) {
    if (allocation_ || bytes == 0 || bytes > 2u * 1024 * 1024) return false;
    size_t allocation_bytes = (bytes + 15u) & ~size_t(15u);
    MemorySnapshot s = memory_snapshot();
    if (!memory_hardware_matches() || s.internal_free < kInternalReserve ||
        s.psram_free < kExternalReserve || allocation_bytes > s.psram_free - kExternalReserve ||
        allocation_bytes > s.psram_largest) return false;
    allocation_ = heap_caps_aligned_alloc(16, allocation_bytes, kExternal);
    if (!allocation_) return false;
    arena.attach(allocation_, bytes);
    return true;
}
void ModelMemory::release() {
    arena.attach(nullptr, 0);
    heap_caps_free(allocation_);
    allocation_ = nullptr;
}

void memory_probe(const char *node) {
    // Diagnostic only: temporary allocation, no trained model or inference claim.
    ModelMemory memory;
    constexpr size_t bytes = 256 * 1024;
    uint32_t start = micros();
    bool passed = memory.reserve(bytes);
    size_t peak = 0;
    if (passed) {
        uint8_t *data = static_cast<uint8_t *>(memory.arena.allocate(bytes,16));
        passed = data != nullptr;
        if (data) {
            for (size_t i=0;i<bytes;++i) data[i]=(uint8_t)(i*17u+3u);
            for (size_t i=0;i<bytes;++i) if(data[i]!=(uint8_t)(i*17u+3u)) { passed=false; break; }
            passed = passed && memory.arena.allocate(1)==nullptr;
            peak = memory.arena.peak();
        }
        memory.release();
    }
    bool heap_ok = heap_caps_check_integrity_all(false);
    Serial.printf("{\"source\":\"HARDWARE_MEASURED\",\"kind\":\"arena_probe\",\"node\":\"%s\","
                  "\"bytes\":%u,\"peak_bytes\":%u,\"elapsed_us\":%lu,\"passed\":%s,\"heap_ok\":%s}\n",
                  node,(unsigned)bytes,(unsigned)peak,(unsigned long)(micros()-start),passed?"true":"false",heap_ok?"true":"false");
}
