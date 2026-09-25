#include <unity.h>
#include <cstdio> // snprintf - needed explicitly under strict native g++/PlatformIO
                  // builds; was silently pulled in transitively before (via
                  // whichever Unity version happened to include <stdio.h>) rather
                  // than declared, so this only surfaced once the REAL `pio test`
                  // was run for the first time, not the ad-hoc g++ compiles used to
                  // hand-verify each fix.
#include "net_allowlist.h"

void setUp(void) {}
void tearDown(void) {}

void test_empty_list_denies_everything(void) {
    NetAllowlist al;
    TEST_ASSERT_FALSE(al.isAllowed("mqtt.example.com"));
    TEST_ASSERT_FALSE(al.isAllowed(NULL));
    TEST_ASSERT_FALSE(al.isAllowed(""));
}

void test_added_host_is_allowed(void) {
    NetAllowlist al;
    TEST_ASSERT_TRUE(al.add("mqtt.example.com"));
    TEST_ASSERT_TRUE(al.isAllowed("mqtt.example.com"));
    TEST_ASSERT_FALSE(al.isAllowed("evil.example.com"));
    TEST_ASSERT_FALSE(al.isAllowed("MQTT.example.com")); // case-sensitive
}

void test_add_rejects_null_and_empty(void) {
    NetAllowlist al;
    TEST_ASSERT_FALSE(al.add(NULL));
    TEST_ASSERT_FALSE(al.add(""));
}

void test_add_rejects_too_long_host(void) {
    NetAllowlist al;
    char too_long[128];
    for (int i = 0; i < 127; i++) too_long[i] = 'a';
    too_long[127] = '\0';
    TEST_ASSERT_FALSE(al.add(too_long));
}

void test_list_full_rejects_further_adds(void) {
    NetAllowlist al;
    char buf[32];
    for (int i = 0; i < NetAllowlist::MAX_ENTRIES; i++) {
        snprintf(buf, sizeof(buf), "host%d.local", i);
        TEST_ASSERT_TRUE(al.add(buf));
    }
    TEST_ASSERT_EQUAL_INT(NetAllowlist::MAX_ENTRIES, al.count());
    TEST_ASSERT_FALSE(al.add("one-too-many.example.com"));
}

void test_clear_resets_and_denies(void) {
    NetAllowlist al;
    al.add("mqtt.example.com");
    al.clear();
    TEST_ASSERT_EQUAL_INT(0, al.count());
    TEST_ASSERT_FALSE(al.isAllowed("mqtt.example.com"));
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_empty_list_denies_everything);
    RUN_TEST(test_added_host_is_allowed);
    RUN_TEST(test_add_rejects_null_and_empty);
    RUN_TEST(test_add_rejects_too_long_host);
    RUN_TEST(test_list_full_rejects_further_adds);
    RUN_TEST(test_clear_resets_and_denies);
    return UNITY_END();
}
