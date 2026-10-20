#pragma once
#include "platform_hal.h"
#include "bounded_arena.h"

struct MemorySnapshot {
    uint32_t internal_free, internal_min, internal_largest;
    uint32_t psram_total, psram_free, psram_largest, stack_free_bytes;
};
MemorySnapshot memory_snapshot();
bool memory_hardware_matches();
void memory_log(const char *node, uint32_t rx_dropped, uint32_t tx_failed);
void memory_probe(const char *node);

/* Explicit allocation only when a real model needs an arena. No fallback to
   scarce internal memory when PSRAM is absent or fragmented. */
class ModelMemory {
public:
    ModelMemory() = default;
    ~ModelMemory() { release(); }
    ModelMemory(const ModelMemory &) = delete;
    ModelMemory &operator=(const ModelMemory &) = delete;
    bool reserve(size_t bytes);
    void release();
    BoundedArena arena;
private:
    void *allocation_ = nullptr;
};
