#pragma once
#include <stdint.h>

// Wraps the ESP32 Arduino core's built-in SNTP client (configTime()) - no
// external library needed. Requires a working network connection
// (WiFi/Ethernet) before calling begin().
class NtpTime {
public:
    // gmt_offset_sec: seconds to add to UTC for local time. 0 is the
    // expected setting here - mqtt_payload's timestamps are meant to be
    // UTC unix time (see mqtt_payload.h) - unless local-time logging is
    // also wanted elsewhere.
    void begin(const char *ntp_server = "pool.ntp.org", long gmt_offset_sec = 0,
               int daylight_offset_sec = 0);

    // Blocks up to timeout_ms waiting for the system clock to sync.
    bool waitForSync(unsigned long timeout_ms = 10000);

    // Current unix timestamp. Meaningless before a successful sync -
    // check isSynced() or waitForSync()'s return first.
    uint32_t nowUnix() const;

    bool isSynced() const;
};
