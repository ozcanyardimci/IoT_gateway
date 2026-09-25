#include <unity.h>
extern "C" {
#include "io_state.h"
}

void setUp(void) {}
void tearDown(void) {}

void test_di_set_get(void) {
    io_state_t s;
    io_state_init(&s);
    uint8_t v;
    TEST_ASSERT_EQUAL_INT(0, io_state_set_di(&s, 3, 1));
    TEST_ASSERT_EQUAL_INT(0, io_state_get_di(&s, 3, &v));
    TEST_ASSERT_EQUAL_UINT8(1, v);
}

void test_di_rejects_out_of_range(void) {
    io_state_t s;
    io_state_init(&s);
    TEST_ASSERT_EQUAL_INT(-1, io_state_set_di(&s, 8, 1));
}

// T2: edge-case coverage added 2026-09-24 - only DI's set-side out-of-range
// check had a test before this. Every getter/setter is bounds- and
// null-checked identically per io_state.c (index < 0, index >= COUNT, NULL
// state, NULL out-param all return -1), but relay/AI and the negative-index
// and null-pointer branches had no direct coverage of their own.

void test_di_rejects_negative_index(void) {
    io_state_t s;
    io_state_init(&s);
    TEST_ASSERT_EQUAL_INT(-1, io_state_set_di(&s, -1, 1));
}

void test_di_get_rejects_null_state(void) {
    uint8_t v;
    TEST_ASSERT_EQUAL_INT(-1, io_state_get_di(NULL, 0, &v));
}

void test_di_get_rejects_null_out_value(void) {
    io_state_t s;
    io_state_init(&s);
    TEST_ASSERT_EQUAL_INT(-1, io_state_get_di(&s, 0, NULL));
}

void test_relay_rejects_out_of_range(void) {
    io_state_t s;
    io_state_init(&s);
    TEST_ASSERT_EQUAL_INT(-1, io_state_set_relay(&s, IO_STATE_RELAY_COUNT, 1));
}

void test_relay_get_rejects_out_of_range(void) {
    io_state_t s;
    io_state_init(&s);
    uint8_t v;
    TEST_ASSERT_EQUAL_INT(-1, io_state_get_relay(&s, IO_STATE_RELAY_COUNT, &v));
}

void test_ai_rejects_out_of_range(void) {
    io_state_t s;
    io_state_init(&s);
    TEST_ASSERT_EQUAL_INT(-1, io_state_set_ai(&s, IO_STATE_AI_COUNT, 100));
}

void test_ai_get_rejects_out_of_range(void) {
    io_state_t s;
    io_state_init(&s);
    uint16_t v;
    TEST_ASSERT_EQUAL_INT(-1, io_state_get_ai(&s, IO_STATE_AI_COUNT, &v));
}

void test_init_rejects_null_state(void) {
    // Must not crash - init() on a NULL state is a no-op per io_state.c.
    io_state_init(NULL);
    TEST_PASS();
}

void test_to_mqtt_state_rejects_null_args(void) {
    io_state_t s;
    io_state_init(&s);
    mqtt_gateway_state_t mst;
    // Neither call should crash - both are no-ops per io_state.c's NULL
    // checks. Nothing to assert on the output; reaching TEST_PASS() without
    // a segfault is the actual assertion here.
    io_state_to_mqtt_state(NULL, 1, &mst);
    io_state_to_mqtt_state(&s, 1, NULL);
    TEST_PASS();
}

void test_to_mqtt_state(void) {
    io_state_t s;
    io_state_init(&s);
    io_state_set_di(&s, 3, 1);
    io_state_set_relay(&s, 1, 1);
    io_state_set_ai(&s, 0, 777);
    io_state_set_ao(&s, 555);

    mqtt_gateway_state_t mst;
    io_state_to_mqtt_state(&s, 42, &mst);
    TEST_ASSERT_EQUAL_UINT32(42, mst.unix_timestamp);
    TEST_ASSERT_EQUAL_UINT8(1, mst.di[3]);
    TEST_ASSERT_EQUAL_UINT8(1, mst.relay[1]);
    TEST_ASSERT_EQUAL_UINT16(777, mst.ai[0]);
    TEST_ASSERT_EQUAL_UINT16(555, mst.ao);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_di_set_get);
    RUN_TEST(test_di_rejects_out_of_range);
    RUN_TEST(test_di_rejects_negative_index);
    RUN_TEST(test_di_get_rejects_null_state);
    RUN_TEST(test_di_get_rejects_null_out_value);
    RUN_TEST(test_relay_rejects_out_of_range);
    RUN_TEST(test_relay_get_rejects_out_of_range);
    RUN_TEST(test_ai_rejects_out_of_range);
    RUN_TEST(test_ai_get_rejects_out_of_range);
    RUN_TEST(test_init_rejects_null_state);
    RUN_TEST(test_to_mqtt_state_rejects_null_args);
    RUN_TEST(test_to_mqtt_state);
    return UNITY_END();
}
