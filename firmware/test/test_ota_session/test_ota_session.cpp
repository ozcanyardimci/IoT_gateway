#include <unity.h>
#include "ota_version_check.h"

void setUp(void) {}
void tearDown(void) {}

void test_version_greater_is_acceptable(void) {
    TEST_ASSERT_TRUE(isOtaVersionAcceptable(2, 1));
}

void test_version_equal_is_rejected(void) {
    TEST_ASSERT_FALSE(isOtaVersionAcceptable(1, 1));
}

void test_version_lower_is_rejected(void) {
    TEST_ASSERT_FALSE(isOtaVersionAcceptable(1, 2));
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_version_greater_is_acceptable);
    RUN_TEST(test_version_equal_is_rejected);
    RUN_TEST(test_version_lower_is_rejected);
    return UNITY_END();
}
