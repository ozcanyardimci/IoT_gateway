#pragma once
#include "ota_trigger.h"
#include "ota_version_check.h"
#include "device_config.h"
#include "net_allowlist.h"
#include "system_watchdog.h"
#include "event_log.h"
#include <NetworkClientSecure.h>

// Orchestrates one OTA update attempt end-to-end, once main.cpp has a
// parsed, syntactically-valid OtaTriggerCommand (ota_trigger.h) ready
// to act on. Ties together every piece built for F4's OTA-trigger
// item: net_allowlist (url_host.h extracts the host) gates the
// download server, DeviceConfig's OTA CA cert configures TLS trust for
// the download itself, HTTPClient + the caller's NetworkClientSecure
// do the actual HTTPS transfer, OtaUpdate (ota_update.h, already built
// and wired for F3's rollback support) writes each chunk to the
// inactive OTA slot, Sha256Streaming (image_verify.h) hashes the same
// bytes as they're written, and verifySignature() (image_verify.h)
// checks the final hash against the command's signature using the
// compiled-in public key (ota_signing_key.h) BEFORE ever calling
// OtaUpdate::end() - i.e. before the newly-written slot becomes
// bootable. See image_verify.h's own header comment for why this is
// done at the application level rather than via ESP-IDF's
// CONFIG_SECURE_SIGNED_APPS_NO_SECURE_BOOT or arduino-esp32's
// Update.installSignature() - both real, documented mechanisms this
// project looked at and ruled out for specific, verified reasons, not
// a style preference.
//
// This function is deliberately NOT unit-testable the way ota_trigger/
// image_verify/url_host are - it's real hardware I/O (HTTPS socket,
// flash writes) with no pure logic left to extract after those three
// modules already pulled out everything that could be. Reviewed by
// hand against every called function's actual signature instead, same
// as main.cpp itself - see docs/roadmap.md's F4 entry for this
// project's standing note on what "verified" means for pieces like
// this, still deferred to the joint `pio run` pass.
//
// Feeds `watchdog` periodically during the (potentially many-second)
// download loop, the same pattern already used by connectNetwork()'s
// and commissioning_portal's own wait loops in main.cpp - a firmware
// download over HTTPS is exactly the kind of legitimately-slow
// operation the watchdog needs to not mistake for a hang. Also
// enforces its OWN stall timeout independent of the watchdog
// (OTA_SESSION_STALL_TIMEOUT_MS below): a connection that stays open
// but stops delivering bytes would otherwise let this loop feed the
// watchdog forever without making progress, defeating the watchdog's
// actual purpose for that specific failure mode.
//
// Returns true only if the full flow succeeded end-to-end AND
// OtaUpdate::end() committed the new image as bootable. Returns false
// on ANY failure - host not allowlisted, no OTA CA configured, bad
// base64 signature, HTTP error, size mismatch, hash mismatch, bad
// signature, flash write error - and every false-returning path after
// OtaUpdate::begin() succeeds calls OtaUpdate::abort() first, so the
// currently-running firmware stays the one that boots next. This
// function never reboots the device itself - that decision belongs to
// the caller (main.cpp), same as ota_update.h's own end()/
// confirmValid() split already keeps that decision separate for the
// rollback-confirm side of OTA.
#define OTA_SESSION_STALL_TIMEOUT_MS 30000 // no new bytes for this
                                            // long -> treat the
                                            // download as dead, not
                                            // just slow
#define OTA_SESSION_WATCHDOG_FEED_INTERVAL_MS 2000

bool runOtaSession(const OtaTriggerCommand &cmd, DeviceConfig &config,
                    NetworkClientSecure &https_client, NetAllowlist &allowlist,
                    SystemWatchdog &watchdog, EventLog &log);
