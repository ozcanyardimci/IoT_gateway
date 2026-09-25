#pragma once
// Minimal native fake for ESP-IDF's esp_system.h - scoped to exactly
// what reset_reason.cpp calls (esp_reset_reason_t's named values it
// switches on, plus esp_reset_reason() itself). No native test actually
// invokes getResetCause() today (power_safety.cpp only uses the
// ResetCause TYPE, not this module's function), but PlatformIO's LDF
// compiles a whole discovered library unconditionally once its header is
// referenced anywhere in the include chain - so this still has to exist
// and compile cleanly for the overall native build to succeed, even
// though nothing currently exercises it. Returns a fixed POWERON reason,
// same "not a functional stand-in" spirit as fake_system_watchdog.cpp.
typedef enum {
    ESP_RST_UNKNOWN,
    ESP_RST_POWERON,
    ESP_RST_EXT,
    ESP_RST_SW,
    ESP_RST_PANIC,
    ESP_RST_INT_WDT,
    ESP_RST_TASK_WDT,
    ESP_RST_WDT,
    ESP_RST_DEEPSLEEP,
    ESP_RST_BROWNOUT,
    ESP_RST_SDIO,
} esp_reset_reason_t;

inline esp_reset_reason_t esp_reset_reason(void) {
    return ESP_RST_POWERON;
}
