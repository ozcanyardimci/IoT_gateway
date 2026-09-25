#pragma once
#include <stdint.h>
#include <stddef.h>

// Persistent event/diagnostic log on LittleFS - a small rotating text
// log (current file + one previous, current rotated to previous once it
// crosses a size cap) so a post-mortem after a crash/brownout/reset has
// something to look at beyond "it rebooted". Deliberately simple: plain
// comma-separated text lines, not a binary format or a real logging
// framework - this is a gateway with a handful of KB of flash to spare
// for this, not a device with a log-shipping pipeline.
//
// unix_ts is caller-supplied (0 = unknown/not synced yet) rather than
// this module reading NtpTime itself, to avoid a dependency between the
// two modules - pass NtpTime::nowUnix() once synced, 0 before that.
// millis()-since-boot is recorded on every line too (see event_log.cpp),
// so entries stay orderable even before NTP sync.
class EventLog {
public:
    // Mounts LittleFS (format-on-fail: losing log history on a corrupt
    // filesystem is acceptable, this isn't safety-critical data) and
    // makes sure /log exists. Returns false if the filesystem still
    // couldn't be mounted - log() calls silently become no-ops in that
    // case rather than crashing the caller.
    bool begin();

    void info(uint32_t unix_ts, const char *tag, const char *message);
    void warn(uint32_t unix_ts, const char *tag, const char *message);
    void error(uint32_t unix_ts, const char *tag, const char *message);

    // Reads back up to out_size-1 bytes of the current log file
    // (null-terminated) - e.g. for a future config/commissioning web UI
    // to display, or for a publish-recent-diagnostics-over-MQTT feature.
    // Returns bytes read (0 if unmounted, unopenable, or empty).
    size_t readCurrent(char *out, size_t out_size);

    bool isMounted() const { return mounted_; }

    static const size_t MAX_LOG_BYTES = 32768;

private:
    void log(uint32_t unix_ts, const char *level, const char *tag, const char *message);
    void rotateIfNeeded();

    bool mounted_ = false;
};
