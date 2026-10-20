/**
 * @file safety_kernel.h
 * @brief Deterministic Safety Kernel for ESP32-S3 Physical Actuation
 */

#ifndef SAFETY_KERNEL_H
#define SAFETY_KERNEL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_MANAGED_DEVICES      10 /* two physical slots + eight virtual slots */
#define DEFAULT_MIN_DWELL_MS     3000   /* 3 seconds minimum dwell time */
#define DEFAULT_MAX_RUN_MS       14400000 /* 4 hours maximum continuous run */
#define DEFAULT_MANUAL_HOLD_MS   300000 /* 5 minutes manual override hold */

typedef enum {
    DEVICE_TYPE_LIGHT = 0,
    DEVICE_TYPE_FAN   = 1,
    DEVICE_TYPE_MOCK  = 2
} device_type_t;

typedef enum {
    ACTION_OFF    = 0,
    ACTION_ON     = 1,
    ACTION_TOGGLE = 2
} action_type_t;

typedef struct {
    char          room[16];
    char          device_id[16];
    device_type_t type;
    int           gpio_pin;
    bool          active_low;
    bool          is_on;
    uint32_t      last_transition_ms;
    bool          has_transition;
    uint32_t      turned_on_ms;
    bool          manual_override;
    uint32_t      manual_override_ms;
} managed_device_t;

void safety_kernel_init(void);

int safety_kernel_register_device(const char *room,
                                  const char *device_id,
                                  device_type_t type,
                                  int gpio_pin,
                                  bool active_low);

bool safety_kernel_request_action(const char *room,
                                  const char *device_id,
                                  action_type_t action,
                                  bool is_manual,
                                  uint32_t current_time_ms,
                                  const char **out_reason);

void safety_kernel_tick_periodic(uint32_t current_time_ms);
bool safety_kernel_release_manual(const char *room, const char *device_id);

const managed_device_t *safety_kernel_get_device(int index);
int safety_kernel_get_device_count(void);

#ifdef __cplusplus
}
#endif

#endif /* SAFETY_KERNEL_H */
