#include "ntp_time.h"
#include <Arduino.h>
#include <time.h>

// A timestamp after this is treated as "real" (post-sync) - the ESP32's
// clock starts near the 1970 epoch before NTP sync, so this threshold
// (year 2023 in unix time) reliably tells synced from unsynced.
#define NTP_SYNC_THRESHOLD_UNIX 1700000000UL

void NtpTime::begin(const char *ntp_server, long gmt_offset_sec, int daylight_offset_sec) {
    configTime(gmt_offset_sec, daylight_offset_sec, ntp_server);
}

bool NtpTime::waitForSync(unsigned long timeout_ms) {
    unsigned long start = millis();
    while (millis() - start < timeout_ms) {
        if (isSynced()) return true;
        delay(100);
    }
    return isSynced();
}

uint32_t NtpTime::nowUnix() const {
    time_t now;
    time(&now);
    return (uint32_t)now;
}

bool NtpTime::isSynced() const {
    return nowUnix() > NTP_SYNC_THRESHOLD_UNIX;
}
