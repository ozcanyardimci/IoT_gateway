#pragma once
#include <stddef.h>
#include <stdint.h>

// Parses the JSON command payload that triggers an OTA update over
// MQTT (main.cpp subscribes to "<device_id>/cmd/ota" - see main.cpp's
// own header comment for the topic/orchestration side of this; this
// file is ONLY the payload parser, kept pure/gcc-testable like F1's
// protocol modules).
//
// Deliberately NOT a general JSON parser - this project has no JSON
// library dependency (mqtt_payload.h builds JSON with snprintf, never
// parses it) and isn't adding one for a single, fixed, flat 5-field
// schema. This hand-rolled parser only understands:
//   {"url":"...","sha256":"...","sig":"...","size":123,"version":"..."}
// as a flat object - no nesting, no escaped characters inside string
// values (a url/sha256/sig/version containing a literal '"' or '\'
// fails the parse rather than being decoded - none of those fields
// legitimately need one). Field order doesn't matter; unrecognized
// extra fields are ignored (forward compatibility), but url/sha256/
// sig/size are REQUIRED - missing or malformed and the whole parse
// fails closed (returns false) rather than partially succeeding. This
// parses untrusted network input (whatever the MQTT broker delivers,
// which - unless the broker itself is compromised or the connection
// to it is - could originate from anyone able to publish to this
// device's command topic), so every extraction is bounds-checked
// against fixed-size buffers, never dynamically allocated.
//
// What this module does NOT do: verify sha256/sig are actually valid
// hex/base64 *content* for a real hash/signature, or that the URL
// points somewhere the allowlist permits, or download/verify/apply
// anything. Syntactic shape only (right length, right character set
// where checkable). See image_verify.h for the cryptographic checks
// and ota_session.h for the allowlist gate and orchestration.

#define OTA_TRIGGER_MAX_URL_LEN 192
#define OTA_TRIGGER_SHA256_HEX_LEN 64 // SHA-256 as lowercase or
                                       // uppercase hex - exactly 64
                                       // characters, not "up to"
// BUG FIX (2026-09-24, L7): this used to be 180, which doesn't match
// ota_session.cpp's actual decode buffer (`uint8_t sig_raw[128]`) -
// every 4 base64 characters decode to 3 raw bytes, so a full 180-char
// value can decode to up to 135 raw bytes, 7 over that buffer.
// base64Decode() (image_verify.cpp, wraps mbedtls_base64_decode) DOES
// bounds-check against out_size and safely refuses rather than
// overflowing - so this was never a memory-safety bug - but it meant
// a syntactically-valid-per-THIS-parser 180-char value would always
// be rejected later by the decode step instead of here, which is
// confusing and makes the two modules' stated limits inconsistent.
// 168 is chosen so the two bounds actually agree: 168 base64 chars
// decode to at most 126 raw bytes (168/4*3), comfortably under the
// 128-byte decode buffer with a couple of bytes to spare - a real
// ECDSA P-256 signature (~96-100 base64 chars) has real headroom
// under 168, same as it did under 180.
#define OTA_TRIGGER_MAX_SIG_B64_LEN 168
#define OTA_TRIGGER_MAX_VERSION_LEN 32
#define OTA_TRIGGER_MAX_IMAGE_SIZE (4UL * 1024UL * 1024UL) // matches
    // firmware/partitions_16mb_ota.csv's app0/app1 size (0x400000) -
    // a sanity bound on the untrusted "size" field, not a substitute
    // for OtaUpdate::begin()'s own real fit check against the actual
    // target partition (ota_session.h calls that too)

struct OtaTriggerCommand {
    char url[OTA_TRIGGER_MAX_URL_LEN];
    char sha256_hex[OTA_TRIGGER_SHA256_HEX_LEN + 1]; // +1 for '\0'
    char sig_b64[OTA_TRIGGER_MAX_SIG_B64_LEN + 1];
    uint32_t size;
    uint32_t version; // monotonic build number, required for downgrade protection
};

// Parses `json` (must be null-terminated) into *out. Returns true only
// if url/sha256/sig/size were all present, well-formed, and fit their
// buffers above - *out is fully populated in that case. Returns false
// otherwise, with *out left in an UNSPECIFIED state (some fields may
// have been written before the failure) - callers must not use *out
// after a false return.
bool ota_trigger_parse(const char *json, OtaTriggerCommand *out);
