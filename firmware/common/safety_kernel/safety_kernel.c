/**
 * @file safety_kernel.c
 * @brief Deterministic Safety Kernel Implementation
 */

#include "safety_kernel.h"
#include <string.h>

#if defined(ESP_PLATFORM) || defined(ARDUINO)
#include "platform_hal.h"
#else
/* Host simulation stubs */
static void pinMode(int pin, int mode) { (void)pin; (void)mode; }
static void digitalWrite(int pin, int val) { (void)pin; (void)val; }
#define OUTPUT 1
#define HIGH 1
#define LOW 0
#endif

static managed_device_t s_devices[MAX_MANAGED_DEVICES];
static int s_device_count = 0;

static void apply_gpio_state(managed_device_t *dev) {
    if (dev->gpio_pin >= 0) {
        int level = dev->is_on ? (dev->active_low ? LOW : HIGH)
                               : (dev->active_low ? HIGH : LOW);
        digitalWrite(dev->gpio_pin, level);
    }
}

void safety_kernel_init(void) {
    s_device_count = 0;
    memset(s_devices, 0, sizeof(s_devices));
}

int safety_kernel_register_device(const char *room,
                                  const char *device_id,
                                  device_type_t type,
                                  int gpio_pin,
                                  bool active_low) {
    if (s_device_count >= MAX_MANAGED_DEVICES || room == NULL || device_id == NULL ||
        !room[0] || !device_id[0] || strlen(room) >= 16 || strlen(device_id) >= 16) {
        return -1;
    }
    for (int i=0; i<s_device_count; ++i) {
        if ((strcmp(s_devices[i].room,room)==0 && strcmp(s_devices[i].device_id,device_id)==0) ||
            (gpio_pin >= 0 && s_devices[i].gpio_pin == gpio_pin)) return -1;
    }

    managed_device_t *dev = &s_devices[s_device_count];
    strncpy(dev->room, room, sizeof(dev->room) - 1);
    strncpy(dev->device_id, device_id, sizeof(dev->device_id) - 1);
    dev->type = type;
    dev->gpio_pin = gpio_pin;
    dev->active_low = active_low;
    dev->is_on = false;
    dev->last_transition_ms = 0;
    dev->has_transition = false;
    dev->turned_on_ms = 0;
    dev->manual_override = false;
    dev->manual_override_ms = 0;

    /* Safe initialization of hardware GPIO to OFF */
    if (gpio_pin >= 0) {
        apply_gpio_state(dev);
        pinMode(gpio_pin, OUTPUT);
    }

    return s_device_count++;
}

bool safety_kernel_request_action(const char *room,
                                  const char *device_id,
                                  action_type_t action,
                                  bool is_manual,
                                  uint32_t current_time_ms,
                                  const char **out_reason) {
    if (room == NULL || device_id == NULL ||
        (action != ACTION_ON && action != ACTION_OFF && action != ACTION_TOGGLE)) {
        if (out_reason) *out_reason = "Invalid room/device/action parameter";
        return false;
    }

    managed_device_t *target = NULL;
    for (int i = 0; i < s_device_count; i++) {
        if (strncmp(s_devices[i].room, room, sizeof(s_devices[i].room)) == 0 &&
            strncmp(s_devices[i].device_id, device_id, sizeof(s_devices[i].device_id)) == 0) {
            target = &s_devices[i];
            break;
        }
    }

    if (target == NULL) {
        if (out_reason) *out_reason = "Device not registered in safety table";
        return false;
    }

    bool target_on = target->is_on;
    if (action == ACTION_ON) {
        target_on = true;
    } else if (action == ACTION_OFF) {
        target_on = false;
    } else if (action == ACTION_TOGGLE) {
        target_on = !target->is_on;
    }

    /* State NOOP check */
    if (target->is_on == target_on) {
        if (is_manual) {
            target->manual_override = true;
            target->manual_override_ms = current_time_ms;
        }
        if (out_reason) *out_reason = "Device already in requested state";
        return true;
    }

    /* Dwell time check (Anti-cycling hysteresis) */
    if (target->has_transition) {
        uint32_t elapsed = current_time_ms - target->last_transition_ms;
        if (elapsed < DEFAULT_MIN_DWELL_MS) {
            if (out_reason) *out_reason = "Dwell time safety limit (rapid cycling prevented)";
            return false;
        }
    }

    /* Manual override lockout check */
    if (!is_manual && target->manual_override) {
        uint32_t hold_elapsed = current_time_ms - target->manual_override_ms;
        if (hold_elapsed < DEFAULT_MANUAL_HOLD_MS) {
            if (out_reason) *out_reason = "Manual override active";
            return false;
        } else {
            target->manual_override = false;
        }
    }

    /* Authorized state change */
    target->is_on = target_on;
    target->last_transition_ms = current_time_ms;
    target->has_transition = true;
    if (target_on) {
        target->turned_on_ms = current_time_ms;
    } else {
        target->turned_on_ms = 0;
    }

    if (is_manual) {
        target->manual_override = true;
        target->manual_override_ms = current_time_ms;
    }

    apply_gpio_state(target);
    if (out_reason) *out_reason = "AUTHORIZED";
    return true;
}

void safety_kernel_tick_periodic(uint32_t current_time_ms) {
    for (int i = 0; i < s_device_count; i++) {
        managed_device_t *dev = &s_devices[i];
        if (dev->manual_override && (uint32_t)(current_time_ms-dev->manual_override_ms)>=DEFAULT_MANUAL_HOLD_MS)
            dev->manual_override=false;
        if (dev->is_on) {
            uint32_t duration = current_time_ms - dev->turned_on_ms;
            if (duration >= DEFAULT_MAX_RUN_MS) {
                /* Safety cutoff exceeded */
                dev->is_on = false;
                dev->turned_on_ms = 0;
                dev->last_transition_ms = current_time_ms;
                dev->has_transition = true;
                dev->manual_override = false;
                apply_gpio_state(dev);
            }
        }
    }
}

const managed_device_t *safety_kernel_get_device(int index) {
    if (index >= 0 && index < s_device_count) {
        return &s_devices[index];
    }
    return NULL;
}

bool safety_kernel_release_manual(const char *room, const char *device_id) {
    if (!room || !device_id) return false;
    for(int i=0;i<s_device_count;++i) {
        if(!strcmp(s_devices[i].room,room) && !strcmp(s_devices[i].device_id,device_id)) {
            s_devices[i].manual_override=false; return true;
        }
    }
    return false;
}

int safety_kernel_get_device_count(void) {
    return s_device_count;
}
