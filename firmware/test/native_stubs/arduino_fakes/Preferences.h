#pragma once
#include <stdint.h>
#include <cstring>
#include <string>
#include <map>

// Native-test fake for Preferences (ESP32 Arduino's NVS wrapper).
//
// Shared/canonical version (2026-09-25) - originally lived only in
// test/test_power_safety/ scoped to just getUChar/putUChar, but
// device_config.cpp (needed to compile at all under `pio test -e native`
// once its header gets pulled in via ota_session.h's include chain) calls
// a much wider slice of the real API: isKey, getBytesLength, getString,
// putString, getUShort/putUShort, getBool/putBool, getUInt/putUInt,
// clear(). Expanded to cover both call sites from one place rather than
// two independently-drifting fakes - see this project's own native-test
// infrastructure notes on why two non-identical fakes of the same class
// name must never end up linked into the same test binary (ODR).
//
// In-process std::map-backed, keyed by "namespace/key" so a value
// written in one begin()/end() session is still there in the next (same
// process) - real enough to test the read-modify-write round-trips both
// callers do, without needing real NVS/flash. Values are stored as
// std::string internally regardless of real type (numbers converted via
// memcpy of their raw bytes) - simplest way to back every put*/get* pair
// with one map without a variant/union.
class Preferences {
public:
    bool begin(const char *name, bool readOnly) {
        (void)readOnly;
        if (fake_force_begin_fail) return false;
        ns_ = name;
        open_ = true;
        return true;
    }

    void end() { open_ = false; }

    bool clear() {
        if (!open_) return false;
        std::string prefix = ns_ + "/";
        for (auto it = store().begin(); it != store().end();) {
            if (it->first.compare(0, prefix.size(), prefix) == 0) it = store().erase(it);
            else ++it;
        }
        return true;
    }

    bool isKey(const char *key) {
        if (!open_) return false;
        return store().find(ns_ + "/" + key) != store().end();
    }

    // --- uint8_t (power_safety's original need) ---
    uint8_t getUChar(const char *key, uint8_t default_value) {
        return getRaw<uint8_t>(key, default_value);
    }
    bool putUChar(const char *key, uint8_t value) {
        return putRaw<uint8_t>(key, value);
    }

    // --- uint16_t (device_config: mqtt_port, wg_port, wg_keepal) ---
    uint16_t getUShort(const char *key, uint16_t default_value) {
        return getRaw<uint16_t>(key, default_value);
    }
    bool putUShort(const char *key, uint16_t value) {
        return putRaw<uint16_t>(key, value);
    }

    // --- uint32_t (device_config: fw_ver) ---
    uint32_t getUInt(const char *key, uint32_t default_value) {
        return getRaw<uint32_t>(key, default_value);
    }
    bool putUInt(const char *key, uint32_t value) {
        return putRaw<uint32_t>(key, value);
    }

    // --- bool (device_config: mqtt_tls) ---
    bool getBool(const char *key, bool default_value) {
        return getRaw<uint8_t>(key, default_value ? 1 : 0) != 0;
    }
    bool putBool(const char *key, bool value) {
        return putRaw<uint8_t>(key, value ? 1 : 0);
    }

    // --- string (device_config: ssid/pass/host/device_id/certs/wg fields) ---
    bool putString(const char *key, const char *value) {
        if (!open_ || value == nullptr) return false;
        store()[ns_ + "/" + key] = value;
        return true;
    }

    // Real Preferences::getString(key, char*, size_t) returns the number
    // of bytes copied (0 on any failure) - device_config.cpp's readString()
    // only calls this after already checking isKey()/getBytesLength()
    // itself, so this doesn't need to duplicate that guarding, just copy
    // faithfully and null-terminate within out_size like the real one does.
    size_t getString(const char *key, char *out, size_t out_size) {
        if (!open_ || out == nullptr || out_size == 0) return 0;
        auto it = store().find(ns_ + "/" + key);
        if (it == store().end()) { out[0] = '\0'; return 0; }
        size_t n = it->second.size();
        if (n >= out_size) n = out_size - 1;
        memcpy(out, it->second.data(), n);
        out[n] = '\0';
        return n;
    }

    // device_config.cpp calls this BEFORE getString() to size-check the
    // caller's buffer - real Preferences::getBytesLength() returns the
    // stored value's length in bytes, 0 if the key doesn't exist. For a
    // string value this is strlen() (no null terminator counted), which
    // is exactly what device_config.cpp's own "needed > out_size" check
    // assumes (out_size includes room for the terminator it adds itself).
    size_t getBytesLength(const char *key) {
        if (!open_) return 0;
        auto it = store().find(ns_ + "/" + key);
        return it == store().end() ? 0 : it->second.size();
    }

    // --- Fake instrumentation (shared across instances/tests, since the
    // real NVS namespace is likewise shared/persistent) ---
    inline static bool fake_force_begin_fail = false;
    static void fake_reset() {
        store().clear();
        fake_force_begin_fail = false;
    }

private:
    template <typename T>
    T getRaw(const char *key, T default_value) {
        if (!open_) return default_value;
        auto it = store().find(ns_ + "/" + key);
        if (it == store().end() || it->second.size() != sizeof(T)) return default_value;
        T v;
        memcpy(&v, it->second.data(), sizeof(T));
        return v;
    }
    template <typename T>
    bool putRaw(const char *key, T value) {
        if (!open_) return false;
        store()[ns_ + "/" + key] = std::string(reinterpret_cast<const char *>(&value), sizeof(T));
        return true;
    }

    static std::map<std::string, std::string> &store() {
        static std::map<std::string, std::string> s;
        return s;
    }
    std::string ns_;
    bool open_ = false;
};
