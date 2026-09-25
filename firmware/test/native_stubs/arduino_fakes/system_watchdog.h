#pragma once
#include <stdint.h>

// Thin wrapper around ESP-IDF's Task Watchdog Timer (esp_task_wdt.h,
// bundled with the framework - no extra dependency, same pattern as
// reset_reason wrapping esp_reset_reason()).
//
// F4's architecture (see docs/roadmap.md Firmware roadmap, F4) runs two
// FreeRTOS tasks: the Arduino main loop (loopTask, already exists,
// implicitly) and a dedicated ModbusMasterTask (modbus_master.h) so a
// slow TLS handshake or OTA flash write can never stall time-critical
// Modbus RTU polling. Both tasks matter for "is this device still
// alive" - a hang in either one is a real fault, not just the main
// loop - so both register themselves and feed independently. Either one
// failing to feed within timeout_ms reboots the device (ESP-IDF's TWDT
// default action: panic + reset), consistent with this project's
// existing reactive-only safety philosophy (power_safety.h: detect and
// recover after the fact, no proactive health prediction attempted).
//
// Deliberately NOT wrapping esp_task_wdt's subscribe-all-idle-tasks
// default behavior or its panic-handler configuration - this class only
// covers what F4 actually needs (begin once, register each real task,
// feed each real task), not the full esp_task_wdt API surface.
class SystemWatchdog {
public:
    // Call once, before any task registers itself. timeout_ms should be
    // comfortably longer than the slowest expected single blocking
    // operation in either task - a TLS handshake (mqtt_tls_config) or an
    // OTA Update.write() chunk are this project's longest known
    // single-call stalls; size this with real headroom above those, not
    // against average-case timing. Returns false if the TWDT couldn't be
    // configured (e.g. already initialized with different parameters by
    // something else in the build - shouldn't happen in this project's
    // own code, but not silently ignored if it does).
    bool begin(uint32_t timeout_ms);

    // Call once, from inside the task that will feed the watchdog (i.e.
    // the calling task IS the one being registered - esp_task_wdt
    // subscribes the current task's handle, there's no "register some
    // other task" call). Returns false on failure (e.g. begin() wasn't
    // called first, or this task is already registered).
    bool addCurrentTask();

    // Call periodically from within a registered task - resets that
    // task's timeout window. Call it every pass of whatever loop that
    // task runs (main loop's loop(), ModbusMasterTask's poll cycle), not
    // just once at the top - a task that adds itself but never actually
    // reaches its own feed() call again is exactly the hang case this
    // exists to catch.
    void feed();
};
