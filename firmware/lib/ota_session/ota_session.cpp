#include "ota_session.h"
#include "ota_version_check.h"
#include "url_host.h"
#include "image_verify.h"
#include "ota_signing_key.h"
#include "ota_update.h"
#include <Arduino.h>
#include <HTTPClient.h>
#include <stdio.h>
#include <string.h>
#include <strings.h> // strcasecmp

namespace {
// static, not stack-local - NetworkClientSecure::setCACert() stores
// the pointer it's given rather than copying it (confirmed against
// its real implementation - see mqtt_tls_config.cpp's identical
// reasoning for the same library call), so this buffer must outlive
// the https_client's use, not just this function's call.
char g_ota_ca_cert[4000]; // matches NVS's own per-string cap, same
                           // sizing reasoning as mqtt_tls_config.cpp's
                           // buffers
} // namespace

bool runOtaSession(const OtaTriggerCommand &cmd, DeviceConfig &config,
                    NetworkClientSecure &https_client, NetAllowlist &allowlist,
                    SystemWatchdog &watchdog, EventLog &log) {
    log.info(0, "ota", "session starting");

    char host[NetAllowlist::MAX_HOST_LEN];
    if (!extractHttpsUrlHost(cmd.url, host, sizeof(host))) {
        log.error(0, "ota", "trigger url is not a valid https:// URL - refusing");
        return false;
    }
    if (!allowlist.isAllowed(host)) {
        log.warn(0, "ota", "download host not in allowlist - refusing");
        return false;
    }

    uint32_t current_version = config.getFirmwareVersion();
    if (!isOtaVersionAcceptable(cmd.version, current_version)) {
        log.error(0, "ota", "trigger version is not strictly greater than current version - refusing downgrade");
        return false;
    }

    if (!config.hasOtaCaCert() || !config.getOtaCaCert(g_ota_ca_cert, sizeof(g_ota_ca_cert))) {
        log.error(0, "ota", "no OTA CA certificate configured - refusing (see device_config.h)");
        return false;
    }
    https_client.setCACert(g_ota_ca_cert);

    // Decode the signature BEFORE downloading anything - a malformed
    // "sig" field means this attempt can never succeed regardless of
    // what gets downloaded, no point pulling megabytes over the air
    // first only to fail at the very end.
    uint8_t sig_raw[128]; // ECDSA P-256 DER signature is ~70-72 raw
                           // bytes (image_verify.h's own choice of
                           // curve) - 128 is real headroom, still well
                           // under RSA-2048's 256 if that's ever used
                           // instead
    int sig_len = base64Decode(cmd.sig_b64, sig_raw, sizeof(sig_raw));
    if (sig_len <= 0) {
        log.error(0, "ota", "sig field is not valid base64 or too long - refusing");
        return false;
    }

    HTTPClient http;
    if (!http.begin(https_client, cmd.url)) {
        log.error(0, "ota", "HTTPClient::begin failed");
        return false;
    }

    int status = http.GET();
    if (status != 200) {
        log.error(0, "ota", "download request did not return HTTP 200");
        http.end();
        return false;
    }

    // cmd.size (from the trigger command - already sanity-bounded by
    // ota_trigger_parse against OTA_TRIGGER_MAX_IMAGE_SIZE) is what
    // sizes the OTA slot and bounds the download loop below, NOT
    // http.getSize()'s server-reported Content-Length, which this
    // code never trusts for anything beyond what it already gets from
    // cmd.size - a server that mismatches its own Content-Length just
    // hits the "size mismatch" check at the end of the loop like any
    // other short/long transfer would.
    OtaUpdate ota;
    if (!ota.begin(cmd.size)) {
        log.error(0, "ota", "OtaUpdate::begin failed - image may not fit the target partition");
        http.end();
        return false;
    }

    Sha256Streaming hasher;
    NetworkClient *stream = http.getStreamPtr();
    uint8_t buf[512];
    uint32_t total_read = 0;
    bool write_failed = false;
    unsigned long last_feed_ms = millis();
    unsigned long last_data_ms = millis();

    while (http.connected() && total_read < cmd.size) {
        size_t available = stream->available();
        if (available == 0) {
            if (millis() - last_data_ms > OTA_SESSION_STALL_TIMEOUT_MS) {
                log.error(0, "ota", "download stalled (no data received) - aborting");
                break;
            }
            if (millis() - last_feed_ms > OTA_SESSION_WATCHDOG_FEED_INTERVAL_MS) {
                watchdog.feed();
                last_feed_ms = millis();
            }
            delay(1);
            continue;
        }

        // BUG FIX (2026-09-24, L5): this used to cap `to_read` only
        // against sizeof(buf), never against how many bytes are still
        // owed to reach cmd.size. A server that keeps streaming past
        // the declared size (misbehaving, or deliberately hostile -
        // this URL only has to be on the allowlist, not trusted) could
        // make a single readBytes() pull write past cmd.size in one
        // shot via ota.write(buf, n) below, before the loop condition
        // (total_read < cmd.size) ever gets a chance to stop it - i.e.
        // flash gets written beyond the size the OTA slot was sized
        // for at ota.begin(cmd.size). Clamp to whatever's left of the
        // declared size first, so a single chunk can never carry the
        // write past cmd.size.
        uint32_t remaining = cmd.size - total_read;
        size_t to_read = available > sizeof(buf) ? sizeof(buf) : available;
        if (to_read > remaining) {
            to_read = remaining;
        }
        size_t n = stream->readBytes(buf, to_read);
        if (n == 0) {
            break; // connection dropped mid-read - loop condition
                    // (http.connected()) catches this next iteration,
                    // but stop immediately rather than spinning once
        }

        if (!ota.write(buf, n)) {
            log.error(0, "ota", "flash write failed mid-download - aborting");
            write_failed = true;
            break;
        }
        hasher.update(buf, n);
        total_read += n;
        last_data_ms = millis();

        if (millis() - last_feed_ms > OTA_SESSION_WATCHDOG_FEED_INTERVAL_MS) {
            watchdog.feed();
            last_feed_ms = millis();
        }
    }
    http.end();

    if (write_failed || total_read != cmd.size) {
        log.error(0, "ota", "download incomplete, stalled, or size mismatch - aborting");
        ota.abort();
        return false;
    }

    uint8_t digest[IMAGE_VERIFY_SHA256_LEN];
    hasher.finish(digest);

    // BUG FIX (2026-09-24, L6): Sha256Streaming now tracks whether
    // every mbedtls call it made internally actually succeeded (see
    // image_verify.h's ok() comment) - check it here and fail closed
    // exactly like every other integrity check in this function,
    // rather than proceeding to compare/trust a digest that mbedtls
    // itself may never have finished computing correctly.
    if (!hasher.ok()) {
        log.error(0, "ota", "internal SHA-256 hashing failure - aborting");
        ota.abort();
        return false;
    }

    char digest_hex[IMAGE_VERIFY_SHA256_LEN * 2 + 1];
    for (int i = 0; i < IMAGE_VERIFY_SHA256_LEN; i++) {
        snprintf(&digest_hex[i * 2], 3, "%02x", digest[i]);
    }

    // Case-insensitive: ota_trigger_parse already validated
    // cmd.sha256_hex is exactly 64 hex characters (upper or lower -
    // isHex() in ota_trigger.cpp accepts both), digest_hex above is
    // always lowercase.
    if (strcasecmp(digest_hex, cmd.sha256_hex) != 0) {
        log.error(0, "ota", "downloaded image hash does not match trigger command - aborting");
        ota.abort();
        return false;
    }

    if (!verifySignature(digest, sig_raw, (size_t)sig_len, OTA_SIGNING_PUBLIC_KEY_PEM)) {
        log.error(0, "ota",
                   "signature verification FAILED - refusing to install "
                   "(tampered image, wrong key, or no real signing key provisioned yet)");
        ota.abort();
        return false;
    }

    if (!ota.end()) {
        log.error(0, "ota", "OtaUpdate::end failed after passing verification - not committed");
        return false; // end() itself leaves nothing committed on
                       // failure - ota_update.h's own documented
                       // contract, nothing more for this function to
                       // undo
    }

    config.setFirmwareVersion(cmd.version);

    log.info(0, "ota", "image verified and committed - reboot required to apply");
    return true;
}
