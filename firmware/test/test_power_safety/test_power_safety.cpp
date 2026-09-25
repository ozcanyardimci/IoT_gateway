#include <unity.h>
#include "power_safety.h"
#include "Preferences.h"

void setUp(void) {
    Preferences::fake_reset();
}
void tearDown(void) {}

void test_streak_zero_no_delay(void) {
    TEST_ASSERT_EQUAL_UINT32(0, powerSafetyBackoffMs(0));
}
void test_streak_one(void) {
    TEST_ASSERT_EQUAL_UINT32(2000, powerSafetyBackoffMs(1));
}
void test_streak_geometric_growth(void) {
    TEST_ASSERT_EQUAL_UINT32(4000, powerSafetyBackoffMs(2));
    TEST_ASSERT_EQUAL_UINT32(8000, powerSafetyBackoffMs(3));
    TEST_ASSERT_EQUAL_UINT32(16000, powerSafetyBackoffMs(4));
    TEST_ASSERT_EQUAL_UINT32(32000, powerSafetyBackoffMs(5));
}
void test_streak_caps_at_60s(void) {
    TEST_ASSERT_EQUAL_UINT32(60000, powerSafetyBackoffMs(6));
    TEST_ASSERT_EQUAL_UINT32(60000, powerSafetyBackoffMs(7));
}
void test_streak_max_uint8_stays_capped_no_overflow(void) {
    TEST_ASSERT_EQUAL_UINT32(60000, powerSafetyBackoffMs(255));
}

// T4: PowerSafety::begin() itself, including its NVS-failure fallback
// path (this is what L1's fix - 2026-09-24 - actually changed: this
// path used to return streak_=0 / no backoff delay on a failed NVS
// open; it now returns 1, matching the "fail toward caution" reasoning
// in power_safety.cpp's own updated comment on this branch).
// Pre-existing behavior, not something this pass changed: a missing
// NVS key defaults its "previous streak" to 1, not 0 (see
// power_safety.cpp's own "hardware-safe default if missing" comment),
// so the very first brownout ever recorded already reads back as 2,
// not 1 - confirmed against the real code path, not assumed.
void test_first_ever_boot_brownout_gives_streak_two(void) {
    PowerSafety ps;
    TEST_ASSERT_EQUAL_UINT8(2, ps.begin(ResetCause::BROWNOUT));
}
void test_non_brownout_boot_resets_streak_to_zero(void) {
    PowerSafety ps1;
    ps1.begin(ResetCause::BROWNOUT); // establish streak=1 in NVS first
    PowerSafety ps2;
    TEST_ASSERT_EQUAL_UINT8(0, ps2.begin(ResetCause::POWER_ON));
}
void test_consecutive_brownouts_increment_streak(void) {
    PowerSafety ps1;
    TEST_ASSERT_EQUAL_UINT8(2, ps1.begin(ResetCause::BROWNOUT)); // see note above
    PowerSafety ps2;
    TEST_ASSERT_EQUAL_UINT8(3, ps2.begin(ResetCause::BROWNOUT));
    PowerSafety ps3;
    TEST_ASSERT_EQUAL_UINT8(4, ps3.begin(ResetCause::BROWNOUT));
}
void test_streak_caps_at_max_tracked(void) {
    PowerSafety ps;
    uint8_t last = 0;
    for (int i = 0; i < 20; i++) {
        PowerSafety p;
        last = p.begin(ResetCause::BROWNOUT);
    }
    TEST_ASSERT_EQUAL_UINT8(8, last); // MAX_TRACKED_STREAK in power_safety.h
    (void)ps;
}
// The specific L1 regression case: NVS itself fails to open.
void test_nvs_open_failure_fails_toward_caution_not_zero(void) {
    Preferences::fake_force_begin_fail = true;
    PowerSafety ps;
    TEST_ASSERT_EQUAL_UINT8(1, ps.begin(ResetCause::BROWNOUT));
    TEST_ASSERT_TRUE(ps.recommendedStartupDelayMs() > 0);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_streak_zero_no_delay);
    RUN_TEST(test_streak_one);
    RUN_TEST(test_streak_geometric_growth);
    RUN_TEST(test_streak_caps_at_60s);
    RUN_TEST(test_streak_max_uint8_stays_capped_no_overflow);
    RUN_TEST(test_first_ever_boot_brownout_gives_streak_two);
    RUN_TEST(test_non_brownout_boot_resets_streak_to_zero);
    RUN_TEST(test_consecutive_brownouts_increment_streak);
    RUN_TEST(test_streak_caps_at_max_tracked);
    RUN_TEST(test_nvs_open_failure_fails_toward_caution_not_zero);
    return UNITY_END();
}
