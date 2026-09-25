#include "ota_update.h"
#include <Update.h>
#include <esp_ota_ops.h>

// Weak-symbol override (see ota_update.h's header comment for why this
// is the one place it may be defined). Deferring the boot-time valid/
// invalid decision that arduino-esp32's initArduino() would otherwise
// make automatically (and, by default, always in favor of "valid" -
// which is what silently defeats rollback if this override isn't
// present).
extern "C" bool verifyRollbackLater() {
    return true;
}

bool OtaUpdate::isPendingVerification() const {
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t state;
    if (esp_ota_get_state_partition(running, &state) != ESP_OK) {
        return false;
    }
    return state == ESP_OTA_IMG_PENDING_VERIFY;
}

void OtaUpdate::confirmValid() {
    esp_ota_mark_app_valid_cancel_rollback();
}

void OtaUpdate::confirmInvalidAndReboot() {
    esp_ota_mark_app_invalid_rollback_and_reboot();
}

bool OtaUpdate::begin(size_t image_size) {
    return Update.begin(image_size);
}

bool OtaUpdate::write(const uint8_t *data, size_t len) {
    // Update::write() takes a non-const uint8_t* even though it only
    // reads from the buffer (computes a hash/writes to flash) - a wart
    // in the library's own signature, not a real mutation, hence the
    // const_cast rather than dropping const from this wrapper's own
    // (better) API.
    return Update.write(const_cast<uint8_t *>(data), len) == len;
}

bool OtaUpdate::end() {
    return Update.end(false);
}

void OtaUpdate::abort() {
    Update.abort();
}

uint8_t OtaUpdate::progressPercent() const {
    return otaProgressPercent(Update.progress(), Update.size());
}
