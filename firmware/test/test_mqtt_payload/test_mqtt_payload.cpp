#include <unity.h>
#include <cstring>
extern "C" {
#include "mqtt_payload.h"
}

void setUp(void) {}
void tearDown(void) {}

void test_json_exact_match(void) {
    mqtt_gateway_state_t st;
    memset(&st, 0, sizeof(st));
    st.unix_timestamp = 1732300000;
    st.di[0] = 1; st.di[4] = 1;
    st.relay[2] = 1;
    st.ai[0] = 512; st.ai[1] = 1023;
    st.ao = 300;
    char out[256];
    int n = mqtt_payload_build_json(&st, out, sizeof(out));
    const char *expect = "{\"ts\":1732300000,\"di\":[1,0,0,0,1,0,0,0],\"relay\":[0,0,1,0],\"ai\":[512,1023],\"ao\":300}";
    TEST_ASSERT_EQUAL_INT((int)strlen(expect), n);
    TEST_ASSERT_EQUAL_STRING(expect, out);
}

void test_rejects_undersized_buffer(void) {
    mqtt_gateway_state_t st;
    memset(&st, 0, sizeof(st));
    char tiny[10];
    int n = mqtt_payload_build_json(&st, tiny, sizeof(tiny));
    TEST_ASSERT_TRUE(n < 0);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_json_exact_match);
    RUN_TEST(test_rejects_undersized_buffer);
    return UNITY_END();
}
