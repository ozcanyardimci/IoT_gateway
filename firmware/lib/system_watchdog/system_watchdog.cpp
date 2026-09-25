#include "system_watchdog.h"
#include <esp_task_wdt.h>

// API confirmed directly against ESP-IDF's real esp_task_wdt.h (v5.3.1,
// matching this project's core-3.x/IDF-5.x assumption elsewhere - see
// mqtt_tls_config.h) rather than assumed from older ESP32 tutorials:
// esp_task_wdt_init() takes an esp_task_wdt_config_t* (timeout_ms,
// idle_core_mask, trigger_panic), not the old (timeout_s, panic) pair
// some pre-IDF-5 examples still show.
//
// arduino-esp32 may already have initialized the TWDT itself by the time
// begin() runs here (its own default Kconfig commonly subscribes the
// idle tasks) - not verified either way for this project's specific
// pioarduino build, so both cases are handled rather than assumed:
// esp_task_wdt_init() failing with ESP_ERR_INVALID_STATE ("already
// initialized") falls back to esp_task_wdt_reconfigure() with the same
// config, which applies whether or not the framework got there first.

bool SystemWatchdog::begin(uint32_t timeout_ms) {
    esp_task_wdt_config_t config = {};
    config.timeout_ms = timeout_ms;
    config.idle_core_mask = 0; // this project doesn't rely on idle-task
                                // monitoring - only the two real tasks
                                // (main loop, ModbusMasterTask) matter
    config.trigger_panic = true;

    esp_err_t err = esp_task_wdt_init(&config);
    if (err == ESP_ERR_INVALID_STATE) {
        err = esp_task_wdt_reconfigure(&config);
    }
    return err == ESP_OK;
}

bool SystemWatchdog::addCurrentTask() {
    return esp_task_wdt_add(nullptr) == ESP_OK;
}

void SystemWatchdog::feed() {
    esp_task_wdt_reset();
}
