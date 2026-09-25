#include <unity.h>
#include <string.h>
#include "url_host.h"

void setUp(void) {}
void tearDown(void) {}

void test_basic_host_with_path(void) {
    char out[64];
    TEST_ASSERT_TRUE(extractHttpsUrlHost("https://fw.example.com/gw-1.4.0.bin", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("fw.example.com", out);
}

void test_host_with_port(void) {
    char out[64];
    TEST_ASSERT_TRUE(extractHttpsUrlHost("https://fw.example.com:8443/x.bin", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("fw.example.com", out);
}

void test_host_with_userinfo(void) {
    char out[64];
    TEST_ASSERT_TRUE(extractHttpsUrlHost("https://user:pass@fw.example.com/x.bin", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("fw.example.com", out);
}

void test_host_with_userinfo_and_evil_domain(void) {
    char out[64];
    TEST_ASSERT_TRUE(extractHttpsUrlHost("https://trusted-host.example:@evil.com/malicious.bin", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("evil.com", out);
}

void test_host_with_no_path(void) {
    char out[64];
    TEST_ASSERT_TRUE(extractHttpsUrlHost("https://fw.example.com", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("fw.example.com", out);
}

void test_host_with_query_string(void) {
    char out[64];
    TEST_ASSERT_TRUE(extractHttpsUrlHost("https://fw.example.com?x=1", out, sizeof(out)));
    TEST_ASSERT_EQUAL_STRING("fw.example.com", out);
}

void test_rejects_plain_http(void) {
    char out[64];
    TEST_ASSERT_FALSE(extractHttpsUrlHost("http://fw.example.com/x.bin", out, sizeof(out)));
}

void test_rejects_no_scheme(void) {
    char out[64];
    TEST_ASSERT_FALSE(extractHttpsUrlHost("fw.example.com/x.bin", out, sizeof(out)));
}

void test_rejects_empty_host(void) {
    char out[64];
    TEST_ASSERT_FALSE(extractHttpsUrlHost("https:///x.bin", out, sizeof(out)));
}

void test_rejects_null_url(void) {
    char out[64];
    TEST_ASSERT_FALSE(extractHttpsUrlHost(NULL, out, sizeof(out)));
}

void test_rejects_too_small_buffer(void) {
    char out[4];
    TEST_ASSERT_FALSE(extractHttpsUrlHost("https://fw.example.com/x.bin", out, sizeof(out)));
}

void test_scheme_lookalike_not_matched_as_prefix(void) {
    // "https://" must be an exact case-sensitive prefix - not e.g.
    // "HTTPS://" or "https:/" (single slash).
    char out[64];
    TEST_ASSERT_FALSE(extractHttpsUrlHost("HTTPS://fw.example.com/x.bin", out, sizeof(out)));
    TEST_ASSERT_FALSE(extractHttpsUrlHost("https:/fw.example.com/x.bin", out, sizeof(out)));
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_basic_host_with_path);
    RUN_TEST(test_host_with_port);
    RUN_TEST(test_host_with_userinfo);
    RUN_TEST(test_host_with_userinfo_and_evil_domain);
    RUN_TEST(test_host_with_no_path);
    RUN_TEST(test_host_with_query_string);
    RUN_TEST(test_rejects_plain_http);
    RUN_TEST(test_rejects_no_scheme);
    RUN_TEST(test_rejects_empty_host);
    RUN_TEST(test_rejects_null_url);
    RUN_TEST(test_rejects_too_small_buffer);
    RUN_TEST(test_scheme_lookalike_not_matched_as_prefix);
    return UNITY_END();
}
