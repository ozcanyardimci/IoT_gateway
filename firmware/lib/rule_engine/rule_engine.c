#include "rule_engine.h"
#include <string.h>

void rule_engine_init(rule_engine_t *engine) {
    if (engine == NULL) return;
    memset(engine, 0, sizeof(*engine));
}

int rule_engine_add_rule(rule_engine_t *engine, const rule_t *rule) {
    if (engine == NULL || rule == NULL) return -1;
    if (engine->rule_count >= RULE_ENGINE_MAX_RULES) return -1;

    if (rule->source == RULE_SOURCE_DI) {
        if (rule->source_index < 0 || rule->source_index >= IO_STATE_DI_COUNT) return -1;
    } else if (rule->source == RULE_SOURCE_AI) {
        if (rule->source_index < 0 || rule->source_index >= IO_STATE_AI_COUNT) return -1;
    } else {
        return -1;
    }
    if (rule->action_relay_index < 0 || rule->action_relay_index >= IO_STATE_RELAY_COUNT) return -1;

    int idx = engine->rule_count;
    engine->rules[idx] = *rule;
    engine->rule_count++;
    return idx;
}

static int evaluate_condition(const rule_t *rule, const io_state_t *state) {
    if (rule->source == RULE_SOURCE_DI) {
        uint8_t v;
        if (io_state_get_di(state, rule->source_index, &v) != 0) return 0;
        if (rule->condition == RULE_COND_DI_EQUALS) return (uint16_t)v == rule->threshold;
        return 0;
    } else {
        uint16_t v;
        if (io_state_get_ai(state, rule->source_index, &v) != 0) return 0;
        if (rule->condition == RULE_COND_AI_ABOVE) return v > rule->threshold;
        if (rule->condition == RULE_COND_AI_BELOW) return v < rule->threshold;
        return 0;
    }
}

void rule_engine_evaluate(const rule_engine_t *engine, const io_state_t *state,
                           int out_relay_commands[4], int out_relay_applied[4]) {
    for (int i = 0; i < 4; i++) {
        out_relay_commands[i] = 0;
        out_relay_applied[i] = 0;
    }
    if (engine == NULL || state == NULL) return;

    for (int i = 0; i < engine->rule_count; i++) {
        const rule_t *r = &engine->rules[i];
        if (!r->enabled) continue;
        if (evaluate_condition(r, state)) {
            out_relay_commands[r->action_relay_index] = r->action_value ? 1 : 0;
            out_relay_applied[r->action_relay_index] = 1;
        }
    }
}
