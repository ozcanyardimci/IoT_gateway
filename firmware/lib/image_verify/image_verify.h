#pragma once
#include <stddef.h>
#include <stdint.h>
#include <mbedtls/sha256.h>

// Cryptographic verification for OTA firmware images - the actual
// security boundary of the OTA trigger flow (ota_trigger.h only
// checks payload SHAPE; net_allowlist only checks the download HOST;
// this is what checks the IMAGE ITSELF is one Ozcan's own offline
// signing key actually produced).
//
// Why app-level verification instead of ESP-IDF's own
// CONFIG_SECURE_SIGNED_APPS_NO_SECURE_BOOT (the documented way to get
// signature-checked OTA without burning Secure Boot eFuses) -
// researched directly, not assumed, and ruled out for two real,
// current reasons:
//   1. That option is a Kconfig setting resolved into the ESP-IDF
//      build - and this project's platformio.ini uses
//      framework = arduino, where (per a PlatformIO community expert's
//      direct answer) the Arduino core ships precompiled and Kconfig
//      changes have NO effect, full stop. pioarduino's fork of the
//      platform (which this project already depends on - see
//      platformio.ini's own header comment) MAY support a
//      `custom_sdkconfig` override, but that could only be confirmed
//      against a third-party auto-generated wiki, with no worked
//      example and an explicit "no examples ... for security-critical
//      flags" caveat of its own - not solid enough ground to build
//      this project's actual security boundary on.
//   2. Separately: arduino-esp32's OWN Update library already ships a
//      signature-verification hook (Update.installSignature()) built
//      on the same ESP-IDF mechanism - and a real, apparently still-
//      open GitHub issue (espressif/arduino-esp32#12422, filed March
//      2026) reports that Update.begin() unconditionally calls
//      reset() internally, which silently discards the installed
//      signature before it's ever checked. The reporter's own words:
//      "I fear there is no workaround until this is fixed." This
//      project's ota_update.cpp already wraps Update.begin()/write()/
//      end() in exactly the call order that bug breaks - relying on
//      installSignature() here would mean trusting a check that
//      silently never runs, which is strictly worse than having no
//      check and knowing it.
//
// This module does the verification itself, in this project's own
// code, using mbedtls directly - already linked into every
// arduino-esp32 build (NetworkClientSecure/TLS depends on it, nothing
// new is being pulled in). ECDSA P-256 (secp256r1) rather than RSA: a
// P-256 signature is ~70-72 raw bytes (~96-100 base64 chars) versus
// RSA-2048's 256 bytes (~344 base64 chars) - meaningfully smaller for
// an MQTT payload constrained by PubSubClient's buffer size, and P-256
// is the same curve arduino-esp32/mbedtls already use pervasively for
// TLS, not an unusual choice for this codebase.
//
// Key handling, stated plainly: the PUBLIC verification key lives in
// firmware SOURCE (ota_signing_key.h, compiled in, not runtime-
// configurable) - deliberately, not stored via DeviceConfig/NVS like
// WireGuard's keys or MQTT's TLS material. If it were commissioning-
// portal-configurable, anyone able to commission this device (an
// unauthenticated local WiFi AP, per commissioning_portal.h's own
// documented caveat) could install THEIR OWN public key and then push
// arbitrary "signed" firmware - defeating the entire point. The
// matching PRIVATE signing key must never touch this repository or
// any device - see ota_signing_key.h for where it lives and how a
// release actually gets signed.

#define IMAGE_VERIFY_SHA256_LEN 32

// Streaming SHA-256 - update() is called once per downloaded chunk (so
// the whole image never needs to be buffered in RAM at once), finish()
// produces the final digest. Thin wrapper around mbedtls's real
// streaming API (mbedtls_sha256_starts/update/finish) - not a hand-
// rolled hash implementation, same reasoning as wireguard_link.h's
// comment about not hand-rolling crypto primitives. mbedtls_sha256_context
// is stored directly (by value), the same way wireguard_link.h stores
// its own library's context structs directly rather than hiding them
// behind a pointer.
class Sha256Streaming {
public:
    Sha256Streaming();
    ~Sha256Streaming();
    void update(const uint8_t *data, size_t len);
    void finish(uint8_t out[IMAGE_VERIFY_SHA256_LEN]);

    // BUG FIX (2026-09-24, L6): mbedtls_sha256_starts/update/finish
    // all return an int status (0 on success, a negative
    // MBEDTLS_ERR_* code on failure - e.g. if the underlying platform
    // primitive it delegates to fails), but this class used to ignore
    // all three return values entirely. In practice these calls don't
    // fail on ESP32's mbedtls build under normal operation, but
    // "don't fail in practice" isn't the same as "checked" - this is
    // the actual integrity check the whole OTA signature scheme rests
    // on (see image_verify.h's own header comment on why this module
    // exists), so an internal hashing failure should never be able to
    // silently produce a digest that then gets treated as trustworthy
    // input to verifySignature(). ok() reports whether every call so
    // far has succeeded; false latches (see .cpp) and never resets.
    // Callers (ota_session.cpp) must check ok() after finish() and
    // fail closed if it's false, same as every other OTA integrity
    // check in that function.
    bool ok() const { return ok_; }

private:
    mbedtls_sha256_context ctx_;
    bool ok_;
};

// Verifies that `signature` (raw bytes, NOT base64 - decode first with
// base64Decode() below) is a valid ECDSA P-256 signature over `hash`
// (exactly IMAGE_VERIFY_SHA256_LEN bytes), checked against the public
// key in `public_key_pem` (a null-terminated PEM string, "-----BEGIN
// PUBLIC KEY-----...").
//
// Returns false on ANY failure - malformed PEM, wrong key type, wrong
// curve, or (the whole point) a signature that doesn't match. Fails
// CLOSED: also returns false if public_key_pem is empty (the
// ota_signing_key.h placeholder before a real key is provisioned)
// rather than treating "no real key configured" as "nothing to check
// against, allow it."
bool verifySignature(const uint8_t hash[IMAGE_VERIFY_SHA256_LEN],
                      const uint8_t *signature, size_t signature_len,
                      const char *public_key_pem);

// Decodes base64 `input` (null-terminated) into `out` (up to out_size
// bytes). Returns the number of decoded bytes on success, or a
// negative value if out_size was too small or input contained invalid
// base64. Thin wrapper around mbedtls_base64_decode - real
// implementation, not hand-rolled (base64 is simple enough that hand-
// rolling it is a much smaller risk than SHA-256/ECDSA would be, but
// there's no reason to duplicate a function mbedtls already provides
// and this firmware already links).
int base64Decode(const char *input, uint8_t *out, size_t out_size);
