#include <unity.h>
extern "C" {
#include "rule_engine.h"
#include "io_state.h"
}

void setUp(void) {}
void tearDown(void) {}

void test_di_equals_triggers_relay(void) {
    rule_engine_t engine;
    rule_engine_init(&engine);
    rule_t r = {RULE_SOURCE_DI, 2, RULE_COND_DI_EQUALS, 1, 0, 1, 1}; // DI2==1 -> relay0 ON
    TEST_ASSERT_EQUAL_INT(0, rule_engine_add_rule(&engine, &r));

    io_state_t state;
    io_state_init(&state);
    io_state_set_di(&state, 2, 1);

    int commands[4], applied[4];
    rule_engine_evaluate(&engine, &state, commands, applied);
    TEST_ASSERT_EQUAL_INT(1, applied[0]);
    TEST_ASSERT_EQUAL_INT(1, commands[0]);
    TEST_ASSERT_EQUAL_INT(0, applied[1]);
}

void test_ai_above_threshold(void) {
    rule_engine_t engine;
    rule_engine_init(&engine);
    rule_t r = {RULE_SOURCE_AI, 0, RULE_COND_AI_ABOVE, 1000, 1, 1, 1}; // AI0>1000 -> relay1 ON
    rule_engine_add_rule(&engine, &r);

    io_state_t state;
    io_state_init(&state);
    io_state_set_ai(&state, 0, 1500);

    int commands[4], applied[4];
    rule_engine_evaluate(&engine, &state, commands, applied);
    TEST_ASSERT_EQUAL_INT(1, applied[1]);
    TEST_ASSERT_EQUAL_INT(1, commands[1]);
}

void test_condition_not_met_no_command(void) {
    rule_engine_t engine;
    rule_engine_init(&engine);
    rule_t r = {RULE_SOURCE_AI, 0, RULE_COND_AI_ABOVE, 1000, 1, 1, 1};
    rule_engine_add_rule(&engine, &r);

    io_state_t state;
    io_state_init(&state);
    io_state_set_ai(&state, 0, 500); // below threshold

    int commands[4], applied[4];
    rule_engine_evaluate(&engine, &state, commands, applied);
    TEST_ASSERT_EQUAL_INT(0, applied[1]);
}

void test_disabled_rule_skipped(void) {
    rule_engine_t engine;
    rule_engine_init(&engine);
    rule_t r = {RULE_SOURCE_DI, 0, RULE_COND_DI_EQUALS, 1, 2, 1, 0}; // enabled=0
    rule_engine_add_rule(&engine, &r);

    io_state_t state;
    io_state_init(&state);
    io_state_set_di(&state, 0, 1);

    int commands[4], applied[4];
    rule_engine_evaluate(&engine, &state, commands, applied);
    TEST_ASSERT_EQUAL_INT(0, applied[2]);
}

void test_later_rule_overrides_earlier_same_relay(void) {
    rule_engine_t engine;
    rule_engine_init(&engine);
    rule_t r1 = {RULE_SOURCE_DI, 0, RULE_COND_DI_EQUALS, 1, 3, 1, 1}; // DI0==1 -> relay3 ON
    rule_t r2 = {RULE_SOURCE_DI, 1, RULE_COND_DI_EQUALS, 1, 3, 0, 1}; // DI1==1 -> relay3 OFF
    rule_engine_add_rule(&engine, &r1);
    rule_engine_add_rule(&engine, &r2);

    io_state_t state;
    io_state_init(&state);
    io_state_set_di(&state, 0, 1);
    io_state_set_di(&state, 1, 1); // both conditions true; r2 added later wins

    int commands[4], applied[4];
    rule_engine_evaluate(&engine, &state, commands, applied);
    TEST_ASSERT_EQUAL_INT(1, applied[3]);
    TEST_ASSERT_EQUAL_INT(0, commands[3]); // r2's OFF wins
}

void test_add_rule_rejects_bad_index(void) {
    rule_engine_t engine;
    rule_engine_init(&engine);
    rule_t bad = {RULE_SOURCE_DI, 99, RULE_COND_DI_EQUALS, 1, 0, 1, 1}; // DI index 99 invalid
    TEST_ASSERT_EQUAL_INT(-1, rule_engine_add_rule(&engine, &bad));
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_di_equals_triggers_relay);
    RUN_TEST(test_ai_above_threshold);
    RUN_TEST(test_condition_not_met_no_command);
    RUN_TEST(test_disabled_rule_skipped);
    RUN_TEST(test_later_rule_overrides_earlier_same_relay);
    RUN_TEST(test_add_rule_rejects_bad_index);
    return UNITY_END();
}
