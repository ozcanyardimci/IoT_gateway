#pragma once
#include <stdint.h>
#include "io_state.h"

#ifdef __cplusplus
extern "C" {
#endif

// Local condition-action automation, independent of MQTT/cloud
// connectivity (docs/roadmap.md Firmware roadmap, F3 - "Local rule
// engine / condition-based actions"). Pure logic, hardware-independent -
// same host-testable pattern as F1.
#define RULE_ENGINE_MAX_RULES 16

typedef enum {
    RULE_SOURCE_DI = 0,
    RULE_SOURCE_AI = 1,
} rule_source_t;

typedef enum {
    RULE_COND_DI_EQUALS = 0, // DI channel == threshold (0 or 1)
    RULE_COND_AI_ABOVE  = 1, // AI channel raw value > threshold
    RULE_COND_AI_BELOW  = 2, // AI channel raw value < threshold
} rule_condition_t;

typedef struct {
    rule_source_t source;
    int source_index;        // DI 0-7 or AI 0-1
    rule_condition_t condition;
    uint16_t threshold;      // 0/1 for DI_EQUALS, raw ADC code for AI_ABOVE/BELOW
    int action_relay_index;  // 0-3
    int action_value;        // 0 = de-energize, 1 = energize
    int enabled;
} rule_t;

typedef struct {
    rule_t rules[RULE_ENGINE_MAX_RULES];
    int rule_count;
} rule_engine_t;

void rule_engine_init(rule_engine_t *engine);

// Returns the added rule's index, or -1 if the engine is full or the
// rule's source_index/action_relay_index is out of range.
int rule_engine_add_rule(rule_engine_t *engine, const rule_t *rule);

// Evaluates all enabled rules against `state`. out_relay_commands[0..3]
// gets each targeted relay's desired value; out_relay_applied[0..3] is 1
// for any relay touched by at least one matching rule, 0 otherwise -
// callers should leave an untouched (applied=0) relay as it is, not force
// it off. Rules are evaluated in add order; a later rule targeting the
// same relay overrides an earlier one (last-write-wins).
void rule_engine_evaluate(const rule_engine_t *engine, const io_state_t *state,
                           int out_relay_commands[4], int out_relay_applied[4]);

#ifdef __cplusplus
}
#endif
