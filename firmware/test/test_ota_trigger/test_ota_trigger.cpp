#include <unity.h>
#include <string.h>
#include <cstdio> // snprintf - see test_net_allowlist.cpp's identical comment
#include "ota_trigger.h"

void setUp(void) {}
void tearDown(void) {}

static const char *VALID = "{\"url\":\"https://fw.example.com/gw-1.4.0.bin\","
                            "\"sha256\":\"a1b2c3d4e5f60718293a4b5c6d7e8f90"
                            "a1b2c3d4e5f60718293a4b5c6d7e8f90\","
                            "\"sig\":\"MEUCIQDabc123\","
                            "\"size\":1048576,\"version\":123}";

void test_valid_payload_parses(void) {
    OtaTriggerCommand cmd;
    TEST_ASSERT_TRUE(ota_trigger_parse(VALID, &cmd));
    TEST_ASSERT_EQUAL_STRING("https://fw.example.com/gw-1.4.0.bin", cmd.url);
    TEST_ASSERT_EQUAL_STRING("a1b2c3d4e5f60718293a4b5c6d7e8f90"
                              "a1b2c3d4e5f60718293a4b5c6d7e8f90", cmd.sha256_hex);
    TEST_ASSERT_EQUAL_STRING("MEUCIQDabc123", cmd.sig_b64);
    TEST_ASSERT_EQUAL_UINT32(1048576, cmd.size);
    TEST_ASSERT_EQUAL_UINT32(123, cmd.version);
}

void test_valid_payload_parses_quoted_version(void) {
    const char *quoted = "{\"url\":\"https://fw.example.com/gw-1.4.0.bin\","
                            "\"sha256\":\"a1b2c3d4e5f60718293a4b5c6d7e8f90"
                            "a1b2c3d4e5f60718293a4b5c6d7e8f90\","
                            "\"sig\":\"MEUCIQDabc123\","
                            "\"size\":1048576,\"version\":\"456\"}";
    OtaTriggerCommand cmd;
    TEST_ASSERT_TRUE(ota_trigger_parse(quoted, &cmd));
    TEST_ASSERT_EQUAL_UINT32(456, cmd.version);
}

void test_field_order_does_not_matter(void) {
    const char *reordered = "{\"size\":42,\"sha256\":\"a1b2c3d4e5f60718293a4b5c6d7e8f90"
                             "a1b2c3d4e5f60718293a4b5c6d7e8f90\",\"url\":\"https://x/y.bin\","
                             "\"sig\":\"AAAA\",\"version\":42}";
    OtaTriggerCommand cmd;
    TEST_ASSERT_TRUE(ota_trigger_parse(reordered, &cmd));
    TEST_ASSERT_EQUAL_UINT32(42, cmd.size);
}

void test_missing_version_fails_closed(void) {
    const char *no_version = "{\"url\":\"https://x/y.bin\",\"sha256\":\"a1b2c3d4e5f60718"
                              "293a4b5c6d7e8f90a1b2c3d4e5f60718293a4b5c6d7e8f90\","
                              "\"sig\":\"AAAA\",\"size\":10}";
    OtaTriggerCommand cmd;
    TEST_ASSERT_FALSE(ota_trigger_parse(no_version, &cmd));
}

void test_unparseable_version_fails_closed(void) {
    const char *bad_version = "{\"url\":\"https://x/y.bin\",\"sha256\":\"a1b2c3d4e5f60718"
                              "293a4b5c6d7e8f90a1b2c3d4e5f60718293a4b5c6d7e8f90\","
                              "\"sig\":\"AAAA\",\"size\":10,\"version\":\"1.4.0\"}";
    OtaTriggerCommand cmd;
    TEST_ASSERT_FALSE(ota_trigger_parse(bad_version, &cmd));
}

void test_negative_version_fails_closed(void) {
    const char *bad_version = "{\"url\":\"https://x/y.bin\",\"sha256\":\"a1b2c3d4e5f60718"
                              "293a4b5c6d7e8f90a1b2c3d4e5f60718293a4b5c6d7e8f90\","
                              "\"sig\":\"AAAA\",\"size\":10,\"version\":-5}";
    OtaTriggerCommand cmd;
    TEST_ASSERT_FALSE(ota_trigger_parse(bad_version, &cmd));
}

void test_missing_url_fails_closed(void) {
    const char *no_url = "{\"sha256\":\"a1b2c3d4e5f60718293a4b5c6d7e8f90"
                          "a1b2c3d4e5f60718293a4b5c6d7e8f90\",\"sig\":\"AAAA\",\"size\":10}";
    OtaTriggerCommand cmd;
    TEST_ASSERT_FALSE(ota_trigger_parse(no_url, &cmd));
}

void test_short_sha256_rejected(void) {
    const char *short_hash = "{\"url\":\"https://x/y.bin\",\"sha256\":\"abcd\","
                              "\"sig\":\"AAAA\",\"size\":10}";
    OtaTriggerCommand cmd;
    TEST_ASSERT_FALSE(ota_trigger_parse(short_hash, &cmd));
}

void test_non_hex_sha256_rejected(void) {
    const char *bad_hash = "{\"url\":\"https://x/y.bin\",\"sha256\":\"zzzzzzzzzzzzzzzz"
                            "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz\","
                            "\"sig\":\"AAAA\",\"size\":10}";
    OtaTriggerCommand cmd;
    TEST_ASSERT_FALSE(ota_trigger_parse(bad_hash, &cmd));
}

void test_zero_size_rejected(void) {
    const char *zero_size = "{\"url\":\"https://x/y.bin\",\"sha256\":\"a1b2c3d4e5f60718"
                             "293a4b5c6d7e8f90a1b2c3d4e5f60718293a4b5c6d7e8f90\","
                             "\"sig\":\"AAAA\",\"size\":0}";
    OtaTriggerCommand cmd;
    TEST_ASSERT_FALSE(ota_trigger_parse(zero_size, &cmd));
}

void test_oversized_size_rejected(void) {
    const char *huge_size = "{\"url\":\"https://x/y.bin\",\"sha256\":\"a1b2c3d4e5f60718"
                             "293a4b5c6d7e8f90a1b2c3d4e5f60718293a4b5c6d7e8f90\","
                             "\"sig\":\"AAAA\",\"size\":99999999}";
    OtaTriggerCommand cmd;
    TEST_ASSERT_FALSE(ota_trigger_parse(huge_size, &cmd));
}

// T6 / L4 regression: the numeric parser used for both "size" and
// "version" must reject anything above UINT32_MAX itself, not rely on
// the host's `unsigned long` happening to be 32 bits. "size" also has
// its own separate OTA_TRIGGER_MAX_IMAGE_SIZE (4MB) check that would
// catch an oversized value regardless of this bug, so "version" (which
// has no such secondary bound) is what actually exercises the fix -
// before it, a value like 4294967296 (2^32) would parse cleanly into
// a 64-bit `unsigned long` on this native/host build and silently
// truncate to 0 on the cast, rather than being rejected.
void test_version_at_uint32_max_accepted(void) {
    const char *at_max = "{\"url\":\"https://x/y.bin\",\"sha256\":\"a1b2c3d4e5f60718"
                          "293a4b5c6d7e8f90a1b2c3d4e5f60718293a4b5c6d7e8f90\","
                          "\"sig\":\"AAAA\",\"size\":1,\"version\":4294967295}";
    OtaTriggerCommand cmd;
    TEST_ASSERT_TRUE(ota_trigger_parse(at_max, &cmd));
    TEST_ASSERT_EQUAL_UINT32(4294967295u, cmd.version);
}
void test_version_one_above_uint32_max_rejected(void) {
    const char *one_over = "{\"url\":\"https://x/y.bin\",\"sha256\":\"a1b2c3d4e5f60718"
                            "293a4b5c6d7e8f90a1b2c3d4e5f60718293a4b5c6d7e8f90\","
                            "\"sig\":\"AAAA\",\"size\":1,\"version\":4294967296}";
    OtaTriggerCommand cmd;
    TEST_ASSERT_FALSE(ota_trigger_parse(one_over, &cmd));
}
void test_version_far_beyond_uint32_max_rejected(void) {
    const char *way_over = "{\"url\":\"https://x/y.bin\",\"sha256\":\"a1b2c3d4e5f60718"
                            "293a4b5c6d7e8f90a1b2c3d4e5f60718293a4b5c6d7e8f90\","
                            "\"sig\":\"AAAA\",\"size\":1,\"version\":99999999999999999999}";
    OtaTriggerCommand cmd;
    TEST_ASSERT_FALSE(ota_trigger_parse(way_over, &cmd));
}
// Same boundary, quoted-string form ("version":"...") - exercises the
// second strtoul-site the L4 fix touched.
void test_quoted_version_one_above_uint32_max_rejected(void) {
    const char *one_over_quoted = "{\"url\":\"https://x/y.bin\",\"sha256\":\"a1b2c3d4e5f60718"
                                   "293a4b5c6d7e8f90a1b2c3d4e5f60718293a4b5c6d7e8f90\","
                                   "\"sig\":\"AAAA\",\"size\":1,\"version\":\"4294967296\"}";
    OtaTriggerCommand cmd;
    TEST_ASSERT_FALSE(ota_trigger_parse(one_over_quoted, &cmd));
}

void test_escape_character_in_string_rejected(void) {
    const char *escaped = "{\"url\":\"https://x/y\\\"z.bin\",\"sha256\":\"a1b2c3d4e5f60718"
                           "293a4b5c6d7e8f90a1b2c3d4e5f60718293a4b5c6d7e8f90\","
                           "\"sig\":\"AAAA\",\"size\":10}";
    OtaTriggerCommand cmd;
    TEST_ASSERT_FALSE(ota_trigger_parse(escaped, &cmd));
}

void test_null_json_rejected(void) {
    OtaTriggerCommand cmd;
    TEST_ASSERT_FALSE(ota_trigger_parse(NULL, &cmd));
}

void test_url_too_long_rejected(void) {
    char json[400];
    char long_url[250];
    memset(long_url, 'a', sizeof(long_url) - 1);
    long_url[sizeof(long_url) - 1] = '\0';
    snprintf(json, sizeof(json), "{\"url\":\"%s\",\"sha256\":\"a1b2c3d4e5f60718293a4b5c6d7e8"
             "f90a1b2c3d4e5f60718293a4b5c6d7e8f90\",\"sig\":\"AAAA\",\"size\":10}", long_url);
    OtaTriggerCommand cmd;
    TEST_ASSERT_FALSE(ota_trigger_parse(json, &cmd));
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_valid_payload_parses);
    RUN_TEST(test_valid_payload_parses_quoted_version);
    RUN_TEST(test_field_order_does_not_matter);
    RUN_TEST(test_missing_version_fails_closed);
    RUN_TEST(test_unparseable_version_fails_closed);
    RUN_TEST(test_negative_version_fails_closed);
    RUN_TEST(test_missing_url_fails_closed);
    RUN_TEST(test_short_sha256_rejected);
    RUN_TEST(test_non_hex_sha256_rejected);
    RUN_TEST(test_zero_size_rejected);
    RUN_TEST(test_oversized_size_rejected);
    RUN_TEST(test_version_at_uint32_max_accepted);
    RUN_TEST(test_version_one_above_uint32_max_rejected);
    RUN_TEST(test_version_far_beyond_uint32_max_rejected);
    RUN_TEST(test_quoted_version_one_above_uint32_max_rejected);
    RUN_TEST(test_escape_character_in_string_rejected);
    RUN_TEST(test_null_json_rejected);
    RUN_TEST(test_url_too_long_rejected);
    return UNITY_END();
}
