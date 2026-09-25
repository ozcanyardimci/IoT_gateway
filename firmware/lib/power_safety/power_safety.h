#pragma once
#include <stdint.h>
#include "reset_reason.h"

// Ties reset_reason's boot-time diagnosis into a persisted "how many
// consecutive boots in a row were brownouts" streak counter, and turns
// that into a simple backoff decision: high-current subsystems (LTE
// modem powerOn(), relay re-activation) shouldn't slam back on
// immediately after a brownout if the supply is marginal and browning
// out repeatedly - that's how you get relay chatter or a boot loop that
// never gets far enough to actually recover.
//
// There is no dedicated power-fail/voltage-sense signal wired into this
// design (checked docs/architecture.md and docs/subsystems/power.md -
// neither mentions one); this module only has the ESP32-S3's own
// internal brownout detector and reset_reason's read of it to work
// with, so "predicting" an imminent brownout isn't possible here, only
// reacting to one that already happened, on the next boot.
//
// The streak itself is persisted in NVS via the Preferences library (see
// power_safety.cpp) so it survives the reboot it's counting - a plain
// RAM variable would reset to 0 on every single brownout, which is
// exactly the case this needs to detect.

// Pure, hardware-independent - safe to call/test directly (no ESP32,
// Arduino, or NVS dependency), same as firmware/lib's other pure-logic
// modules. Geometric backoff shaped like mqtt_client_wrapper's reconnect
// backoff: 2s, 4s, 8s, 16s, 32s, capped at 60s. streak=0 means "not
// currently brownout-looping" - no extra delay recommended.
inline unsigned long powerSafetyBackoffMs(uint8_t streak) {
    if (streak == 0) return 0;
    uint8_t shift = (uint8_t)((streak - 1 < 5) ? (streak - 1) : 5);
    unsigned long delay_ms = 2000UL << shift; // max 2000<<5 = 64000, no overflow risk
    const unsigned long MAX_DELAY_MS = 60000UL;
    return delay_ms > MAX_DELAY_MS ? MAX_DELAY_MS : delay_ms;
}

class PowerSafety {
public:
    // Call once at startup, right after reset_reason's getResetCause().
    // Reads/updates a persisted streak counter in NVS: increments on a
    // brownout boot, resets to 0 on any other boot cause. Returns the
    // streak count *after* this boot is accounted for (so a first-ever
    // brownout returns 1). If NVS itself can't be opened, fails toward
    // CAUTION (streak 1, one backoff step) rather than assuming
    // everything is fine - a failing NVS open is itself a plausible
    // brownout symptom (see power_safety.cpp's comment on this branch).
    uint8_t begin(ResetCause cause);

    // How long the caller should hold off before energizing anything
    // power-hungry, given the streak this boot observed. 0 = no extra
    // delay needed.
    unsigned long recommendedStartupDelayMs() const { return powerSafetyBackoffMs(streak_); }

    uint8_t brownoutStreak() const { return streak_; }

private:
    uint8_t streak_ = 0;
    static const uint8_t MAX_TRACKED_STREAK = 8;
};
