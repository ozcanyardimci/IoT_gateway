#include "power_safety.h"
#include <Preferences.h>

namespace {
const char *NVS_NAMESPACE = "pwrsafety";
const char *STREAK_KEY = "streak";
} // namespace

uint8_t PowerSafety::begin(ResetCause cause) {
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, false)) {
        // BUG FIX (2026-09-24): this used to fail safe to streak_ = 0
        // (no backoff delay) when NVS itself won't open. That's the
        // wrong direction for a module whose whole purpose is reacting
        // to marginal power conditions - a failing NVS open is itself
        // a plausible symptom of exactly that (brownout mid-flash-
        // write, etc.), so this now fails toward CAUTION instead:
        // treat it the same as the "no streak recorded yet" case below
        // (prev==255 -> 1), applying one backoff step rather than none.
        streak_ = 1;
        return streak_;
    }
    uint8_t prev = prefs.getUChar(STREAK_KEY, 255);
    if (prev == 255) prev = 1; // hardware-safe default if missing
    if (cause == ResetCause::BROWNOUT) {
        streak_ = (prev < MAX_TRACKED_STREAK) ? (uint8_t)(prev + 1) : MAX_TRACKED_STREAK;
    } else {
        streak_ = 0;
    }
    prefs.putUChar(STREAK_KEY, streak_);
    prefs.end();
    return streak_;
}
