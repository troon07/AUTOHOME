/**
 * @file neural_agent.h
 * @brief Bounded deterministic text baseline; legacy API name, no neural model.
 */

#ifndef NEURAL_AGENT_H
#define NEURAL_AGENT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NEURAL_EMBED_DIM 64

typedef enum {
    INTENT_UNKNOWN = 0,
    INTENT_DEVICE_CONTROL = 1,
    INTENT_SCENE_SLEEP = 2,
    INTENT_SCENE_AWAY = 3,
    INTENT_STATUS_QUERY = 4,
    INTENT_REPAIR = 5
} neural_intent_type_t;

typedef struct {
    neural_intent_type_t intent;
    char                 action[16];   /* "ON", "OFF", "TOGGLE", "NIGHT_MODE" */
    char                 room[16];     /* "bedroom", "living_room", "kitchen", "office", "ALL" */
    char                 device[16];   /* "light", "fan", "ALL" */
    float                confidence;
    bool                 requires_clarification;
    char                 clarification_prompt[64];
    char                 spoken_response[128];
} neural_parse_result_t;

void neural_agent_init(void);
bool neural_agent_selftest(void);

void neural_agent_process(const char *input_text,
                          const char *active_room,
                          neural_parse_result_t *out_result);

#ifdef __cplusplus
}
#endif

#endif /* NEURAL_AGENT_H */
