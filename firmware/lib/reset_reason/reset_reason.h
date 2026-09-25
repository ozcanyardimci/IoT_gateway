#pragma once

// Thin wrapper around ESP-IDF's esp_reset_reason() (bundled with the
// framework - no extra dependency). Relay outputs already default to
// de-energized on every boot regardless of reset cause
// (RelayOutputs::begin(), relay_driver.h) - the actual "safe state"
// behavior doesn't depend on this module. This is for detection/logging/
// status-LED purposes, so a brownout can be told apart from a normal
// power-on or a firmware-triggered reset.
enum class ResetCause {
    POWER_ON,
    BROWNOUT,
    SOFTWARE,
    WATCHDOG,
    OTHER,
};

ResetCause getResetCause();
const char *resetCauseToString(ResetCause cause);
