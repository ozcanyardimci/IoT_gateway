#include <unity.h>
#include "ota_update.h"
#include "Update.h"
#include "esp_ota_ops.h"

UpdateClass Update;

esp_partition_t* fake_running_partition = nullptr;
esp_err_t fake_get_state_return = ESP_OK;
esp_ota_img_states_t fake_partition_state = ESP_OTA_IMG_UNDEFINED;
bool fake_mark_app_valid_called = false;
bool fake_mark_app_invalid_called = false;

const esp_partition_t* esp_ota_get_running_partition(void) {
    return fake_running_partition;
}

esp_err_t esp_ota_get_state_partition(const esp_partition_t *partition, esp_ota_img_states_t *ota_state) {
    if (partition != fake_running_partition) return ESP_FAIL;
    *ota_state = fake_partition_state;
    return fake_get_state_return;
}

esp_err_t esp_ota_mark_app_valid_cancel_rollback(void) {
    fake_mark_app_valid_called = true;
    return ESP_OK;
}

esp_err_t esp_ota_mark_app_invalid_rollback_and_reboot(void) {
    fake_mark_app_invalid_called = true;
    return ESP_OK;
}

void fake_esp_ota_ops_reset(void) {
    fake_running_partition = nullptr;
    fake_get_state_return = ESP_OK;
    fake_partition_state = ESP_OTA_IMG_UNDEFINED;
    fake_mark_app_valid_called = false;
    fake_mark_app_invalid_called = false;
}

void setUp(void) {
    Update.fake_reset();
    fake_esp_ota_ops_reset();
}
void tearDown(void) {}

void test_zero_total_is_zero_percent(void) {
    TEST_ASSERT_EQUAL_UINT8(0, otaProgressPercent(0, 0));
}
void test_zero_done_is_zero_percent(void) {
    TEST_ASSERT_EQUAL_UINT8(0, otaProgressPercent(0, 1000));
}
void test_half_done(void) {
    TEST_ASSERT_EQUAL_UINT8(50, otaProgressPercent(500, 1000));
}
void test_fully_done(void) {
    TEST_ASSERT_EQUAL_UINT8(100, otaProgressPercent(1000, 1000));
}
void test_done_exceeds_total_clamps_to_100(void) {
    TEST_ASSERT_EQUAL_UINT8(100, otaProgressPercent(1001, 1000));
}
void test_rounds_down_not_up(void) {
    TEST_ASSERT_EQUAL_UINT8(99, otaProgressPercent(999, 1000));
}
void test_realistic_4mb_image_partial(void) {
    TEST_ASSERT_EQUAL_UINT8(25, otaProgressPercent(1048576, 4194304));
}

void test_is_pending_verification_true(void) {
    OtaUpdate ota;
    fake_running_partition = (esp_partition_t*)0x1234;
    fake_partition_state = ESP_OTA_IMG_PENDING_VERIFY;
    fake_get_state_return = ESP_OK;
    TEST_ASSERT_TRUE(ota.isPendingVerification());
}

void test_is_pending_verification_false_if_valid(void) {
    OtaUpdate ota;
    fake_running_partition = (esp_partition_t*)0x1234;
    fake_partition_state = ESP_OTA_IMG_VALID;
    fake_get_state_return = ESP_OK;
    TEST_ASSERT_FALSE(ota.isPendingVerification());
}

void test_confirm_valid_calls_esp_idf(void) {
    OtaUpdate ota;
    ota.confirmValid();
    TEST_ASSERT_TRUE(fake_mark_app_valid_called);
}

void test_begin_calls_update(void) {
    OtaUpdate ota;
    Update.fake_begin_return_ = true;
    TEST_ASSERT_TRUE(ota.begin(1024));
    TEST_ASSERT_TRUE(Update.fake_begin_called_);
    TEST_ASSERT_EQUAL_UINT32(1024, Update.fake_size_);
}

void test_write_calls_update(void) {
    OtaUpdate ota;
    uint8_t buf[10] = {0};
    TEST_ASSERT_TRUE(ota.write(buf, sizeof(buf)));
    TEST_ASSERT_TRUE(Update.fake_write_called_);
    TEST_ASSERT_EQUAL_UINT32(10, Update.fake_bytes_written_);
}

void test_end_calls_update(void) {
    OtaUpdate ota;
    Update.fake_end_return_ = true;
    TEST_ASSERT_TRUE(ota.end());
    TEST_ASSERT_TRUE(Update.fake_end_called_);
    TEST_ASSERT_FALSE(Update.fake_end_arg_);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_zero_total_is_zero_percent);
    RUN_TEST(test_zero_done_is_zero_percent);
    RUN_TEST(test_half_done);
    RUN_TEST(test_fully_done);
    RUN_TEST(test_done_exceeds_total_clamps_to_100);
    RUN_TEST(test_rounds_down_not_up);
    RUN_TEST(test_realistic_4mb_image_partial);
    RUN_TEST(test_is_pending_verification_true);
    RUN_TEST(test_is_pending_verification_false_if_valid);
    RUN_TEST(test_confirm_valid_calls_esp_idf);
    RUN_TEST(test_begin_calls_update);
    RUN_TEST(test_write_calls_update);
    RUN_TEST(test_end_calls_update);
    return UNITY_END();
}
