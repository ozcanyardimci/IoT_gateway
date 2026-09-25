#pragma once
#include <stdint.h>

// Pulled out of ota_session.h/.cpp (2026-09) so the native test build for
// this one pure comparison doesn't need to fake NetworkClientSecure,
// HTTPClient, or mbedtls just to reach it - those three are only needed by
// runOtaSession() itself, which does real HTTPS I/O and flash writes and is
// documented (ota_session.cpp's own header comment) as reviewed by hand,
// not natively unit-tested. This function has zero dependency on any of
// that, so splitting it into its own library is the same fix already
// applied to mqtt_outbox (pulled out of mqtt_client_wrapper for the
// identical reason).

// True if `declared_version` (from an incoming OTA trigger command) is
// strictly newer than `current_version` (the device's own running
// firmware version) - i.e. whether this OTA should proceed at all.
bool isOtaVersionAcceptable(uint32_t declared_version, uint32_t current_version);
