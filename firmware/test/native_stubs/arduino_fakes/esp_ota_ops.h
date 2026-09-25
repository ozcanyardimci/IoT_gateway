#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL -1

typedef struct esp_partition_t esp_partition_t;

typedef enum {
    ESP_OTA_IMG_NEW = 0,
    ESP_OTA_IMG_PENDING_VERIFY,
    ESP_OTA_IMG_VALID,
    ESP_OTA_IMG_INVALID,
    ESP_OTA_IMG_ABORTED,
    ESP_OTA_IMG_UNDEFINED
} esp_ota_img_states_t;

// Fakes
extern esp_partition_t* fake_running_partition;
extern esp_err_t fake_get_state_return;
extern esp_ota_img_states_t fake_partition_state;

extern bool fake_mark_app_valid_called;
extern bool fake_mark_app_invalid_called;

const esp_partition_t* esp_ota_get_running_partition(void);
esp_err_t esp_ota_get_state_partition(const esp_partition_t *partition, esp_ota_img_states_t *ota_state);

esp_err_t esp_ota_mark_app_valid_cancel_rollback(void);
esp_err_t esp_ota_mark_app_invalid_rollback_and_reboot(void);

void fake_esp_ota_ops_reset(void);

#ifdef __cplusplus
}
#endif
