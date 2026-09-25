#include "event_log.h"
#include <LittleFS.h>
#include <Arduino.h>

namespace {
const char *LOG_DIR = "/log";
const char *CURRENT_PATH = "/log/current.log";
const char *PREV_PATH = "/log/prev.log";
} // namespace

bool EventLog::begin() {
    mounted_ = LittleFS.begin(true);
    if (mounted_ && !LittleFS.exists(LOG_DIR)) {
        LittleFS.mkdir(LOG_DIR);
    }
    return mounted_;
}

void EventLog::info(uint32_t unix_ts, const char *tag, const char *message) {
    log(unix_ts, "INFO", tag, message);
}

void EventLog::warn(uint32_t unix_ts, const char *tag, const char *message) {
    log(unix_ts, "WARN", tag, message);
}

void EventLog::error(uint32_t unix_ts, const char *tag, const char *message) {
    log(unix_ts, "ERROR", tag, message);
}

void EventLog::log(uint32_t unix_ts, const char *level, const char *tag, const char *message) {
    if (!mounted_) return;
    File f = LittleFS.open(CURRENT_PATH, "a", true);
    if (!f) return;
    f.printf("%lu,%lu,%s,%s,\"", (unsigned long)unix_ts, (unsigned long)millis(), level, tag);
    for (size_t i = 0; message[i] != '\0'; i++) {
        char c = message[i];
        if (c == '"') f.print("\"\"");
        else f.print(c);
    }
    f.print("\"\n");
    f.close();
    rotateIfNeeded();
}

void EventLog::rotateIfNeeded() {
    File f = LittleFS.open(CURRENT_PATH, "r");
    if (!f) return;
    size_t sz = f.size();
    f.close();
    if (sz < MAX_LOG_BYTES) return;
    LittleFS.remove(PREV_PATH);
    LittleFS.rename(CURRENT_PATH, PREV_PATH);
}

size_t EventLog::readCurrent(char *out, size_t out_size) {
    if (!mounted_ || out == nullptr || out_size == 0) return 0;
    File f = LittleFS.open(CURRENT_PATH, "r");
    if (!f) {
        out[0] = '\0';
        return 0;
    }
    size_t n = f.read((uint8_t *)out, out_size - 1);
    out[n] = '\0';
    f.close();
    return n;
}
