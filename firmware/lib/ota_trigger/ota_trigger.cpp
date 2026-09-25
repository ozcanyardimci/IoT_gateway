#include "ota_trigger.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

namespace {

// Finds `"key"` followed by optional whitespace and a `:` in json.
// Returns a pointer just past the ':' (and any whitespace after it),
// or nullptr if the key isn't present in that exact quoted form.
// Deliberately simple substring search, not a real tokenizer - a
// value elsewhere in the payload that happens to contain the literal
// text "key" won't false-match because the search string includes the
// surrounding quotes and colon punctuation, not just the bare key
// name.
const char *findValueStart(const char *json, const char *key) {
    char needle[40];
    int n = snprintf(needle, sizeof(needle), "\"%s\"", key);
    if (n <= 0 || (size_t)n >= sizeof(needle)) return nullptr;

    const char *p = strstr(json, needle);
    if (p == nullptr) return nullptr;
    p += n;

    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    if (*p != ':') return nullptr;
    p++;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    return p;
}

// Extracts a quoted string value starting at *p (*p must point at the
// opening '"'). Fails (returns false) on: no opening quote, no closing
// quote before the payload ends, an escape character '\' anywhere in
// the value (this project's fields never legitimately need one - a
// URL/hex-hash/base64-signature/version string containing a literal
// backslash or embedded quote isn't a value this parser accepts,
// rather than trying to implement JSON's real escape handling for a
// hand-rolled parser this narrow), or a value that doesn't fit
// out_size (including the null terminator).
bool extractString(const char *p, char *out, size_t out_size) {
    if (p == nullptr || *p != '"') return false;
    p++;
    size_t len = 0;
    while (*p != '"') {
        if (*p == '\0' || *p == '\\') return false;
        if (len + 1 >= out_size) return false; // +1: leave room for '\0'
        out[len++] = *p++;
    }
    out[len] = '\0';
    return len > 0; // empty string ("") is treated as "field missing"
}

// BUG FIX (2026-09-24, L4): this used to parse with strtoul() into an
// `unsigned long` and cast straight to uint32_t. `unsigned long` is
// only guaranteed to be AT LEAST 32 bits - it's 32 bits on the real
// ESP32-S3 target, but 64 bits on this project's native/host test
// build (and on most 64-bit desktop platforms generally). On a 64-bit
// host, a crafted value larger than UINT32_MAX would parse cleanly
// into the wider `unsigned long` (no saturation), then get silently
// truncated by the cast - potentially wrapping into a small number
// that slips past the caller's own bounds check (e.g. "size" against
// OTA_TRIGGER_MAX_IMAGE_SIZE). Fixed by parsing into `unsigned long
// long` (guaranteed >=64 bits on every real platform, C99/C++11) and
// explicitly rejecting anything above UINT32_MAX before the cast -
// this makes the bound check itself independent of the host's
// `unsigned long` width, rather than accidentally relying on it.
bool extractUint(const char *p, uint32_t *out) {
    if (p == nullptr) return false;
    if (*p < '0' || *p > '9') return false; // reject a leading '-' too -
                                              // size is never negative
    char *end = nullptr;
    unsigned long long v = strtoull(p, &end, 10);
    if (end == p) return false;
    if (v > 0xFFFFFFFFULL) return false; // explicit uint32_t bound,
                                          // independent of host
                                          // `unsigned long` width
    *out = (uint32_t)v;
    return true;
}

bool isHex(const char *s, size_t expected_len) {
    if (strlen(s) != expected_len) return false;
    for (size_t i = 0; i < expected_len; i++) {
        char c = s[i];
        bool ok = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
        if (!ok) return false;
    }
    return true;
}

} // namespace

bool ota_trigger_parse(const char *json, OtaTriggerCommand *out) {
    if (json == nullptr || out == nullptr) return false;

    if (!extractString(findValueStart(json, "url"), out->url, sizeof(out->url))) {
        return false;
    }
    if (!extractString(findValueStart(json, "sha256"), out->sha256_hex, sizeof(out->sha256_hex))) {
        return false;
    }
    if (!isHex(out->sha256_hex, OTA_TRIGGER_SHA256_HEX_LEN)) {
        return false;
    }
    if (!extractString(findValueStart(json, "sig"), out->sig_b64, sizeof(out->sig_b64))) {
        return false;
    }
    if (!extractUint(findValueStart(json, "size"), &out->size)) {
        return false;
    }
    if (out->size == 0 || out->size > OTA_TRIGGER_MAX_IMAGE_SIZE) {
        return false;
    }

    const char* version_p = findValueStart(json, "version");
    if (!version_p) {
        return false;
    }
    
    // It could be a number `123` or a string `"123"`.
    if (*version_p == '"') {
        char ver_str[32];
        if (!extractString(version_p, ver_str, sizeof(ver_str))) return false;
        if (ver_str[0] < '0' || ver_str[0] > '9') return false;
        char *end = nullptr;
        unsigned long long v = strtoull(ver_str, &end, 10); // see L4 fix note above
        if (end == ver_str || *end != '\0') return false;
        if (v > 0xFFFFFFFFULL) return false;
        out->version = (uint32_t)v;
    } else {
        if (!extractUint(version_p, &out->version)) return false;
    }

    return true;
}
