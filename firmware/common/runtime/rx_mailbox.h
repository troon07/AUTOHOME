#pragma once
#include "platform_hal.h"
#include <freertos/queue.h>
#include "clawgrid_protocol.h"

struct RxFrame {
    uint8_t mac[6];
    uint16_t length;
    uint8_t bytes[CLAWGRID_MAX_PACKET];
};

/* ESP-NOW callback executes in the Wi-Fi task, not an ISR. Copy then return.
   Queue storage and bookkeeping stay in internal static RAM. */
class RxMailbox {
public:
    void begin() { queue_ = xQueueCreateStatic(kDepth, sizeof(RxFrame), storage_, &control_); }
    void push(const uint8_t *mac, const uint8_t *data, int length) {
        if (!queue_ || !mac || !data || length < CLAWGRID_HEADER_SIZE || length > CLAWGRID_MAX_PACKET) {
            drop(); return;
        }
        RxFrame frame = {};
        memcpy(frame.mac, mac, 6); frame.length = (uint16_t)length;
        memcpy(frame.bytes, data, length);
        if (xQueueSend(queue_, &frame, 0) != pdTRUE) drop();
    }
    bool pop(RxFrame &frame) { return queue_ && xQueueReceive(queue_, &frame, 0) == pdTRUE; }
    uint32_t dropped() {
        portENTER_CRITICAL(&mux_); uint32_t n = dropped_; portEXIT_CRITICAL(&mux_); return n;
    }
    static constexpr size_t kDepth = 8;
private:
    void drop() { portENTER_CRITICAL(&mux_); ++dropped_; portEXIT_CRITICAL(&mux_); }
    QueueHandle_t queue_ = nullptr;
    StaticQueue_t control_ = {};
    alignas(4) uint8_t storage_[kDepth * sizeof(RxFrame)] = {};
    portMUX_TYPE mux_ = portMUX_INITIALIZER_UNLOCKED;
    uint32_t dropped_ = 0;
};
