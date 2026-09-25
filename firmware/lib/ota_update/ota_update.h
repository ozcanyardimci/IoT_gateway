#pragma once
#include <stdint.h>
#include <stddef.h>

// OTA firmware update with real rollback support - F3's "OTA update
// with rollback" item. Mechanics confirmed directly against
// arduino-esp32's own source (cores/esp32/esp32-hal-misc.c) and
// ESP-IDF's OTA docs, not assumed:
//
// - The prebuilt Arduino-core bootloader (both the official core and
//   this project's pioarduino platform) already ships with
//   CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE on - no custom bootloader
//   build needed, unlike wireguard_link's situation.
// - BUT by default, arduino-esp32's initArduino() auto-confirms every
//   boot as valid before setup() even runs (a weak verifyOta() that
//   defaults to true, invoked because the weak verifyRollbackLater()
//   defaults to false) - meaning rollback support is silently a no-op
//   unless the app overrides that default. This project's
//   ota_update.cpp defines verifyRollbackLater() to return true,
//   deferring that decision to this module's confirmValid()/
//   confirmInvalidAndReboot() instead - only ONE definition of that
//   weak symbol may exist across the whole firmware link, so it lives
//   here and nowhere else.
// - Past that override, the actual safety net is the bootloader's own
//   built-in behavior, not this module's code: if the app never calls
//   confirmValid() (or confirmInvalidAndReboot()) - crashes, hangs,
//   browns out before confirming - the bootloader detects the
//   still-pending state on the NEXT boot and automatically reverts to
//   the previously-working slot. This module's job is providing the
//   image-write path and exposing that confirm/reject decision to the
//   app's own health check, not implementing the revert itself.
// - Needs a partition table with two OTA app slots + an otadata
//   partition (ota_0/ota_1/otadata). This board's stock default
//   partition table already has them (sized for 8MB flash); this
//   project now ships firmware/partitions_16mb_ota.csv sized for the
//   real 16MB N16R8 module instead - see that file's own header for
//   the sizing/alignment reasoning, and platformio.ini's
//   board_build.partitions line.
//
// Pure progress-percentage math is a free function (otaProgressPercent
// below) so it's gcc-testable without any ESP32/Update dependency -
// see firmware/test/test_ota_update.

// Pure, hardware-independent. Clamps to 100 rather than ever reporting
// over it (done could in principle reach/exceed total on the last
// write depending on how a caller chunks things).
inline uint8_t otaProgressPercent(size_t done, size_t total) {
    if (total == 0) return 0;
    if (done >= total) return 100;
    return (uint8_t)((done * 100) / total);
}

class OtaUpdate {
public:
    // True if this boot is a fresh OTA image still waiting on
    // confirmValid()/confirmInvalidAndReboot() - i.e. the bootloader
    // hasn't rolled it back (yet) but hasn't been told it's good
    // either. False on an ordinary boot (nothing pending) or if the
    // OTA state can't be read.
    bool isPendingVerification() const;

    // Call once the app has confirmed the new firmware actually works
    // (whatever that check means for this device - WiFi/MQTT
    // connecting, sensors reading sane values, whatever's meaningful
    // here). Cancels the pending rollback - the bootloader boots this
    // same image normally from now on.
    void confirmValid();

    // Call if the app itself determines the new firmware is broken
    // (rather than waiting for a crash/hang to trigger the
    // bootloader's own automatic rollback on next boot). Reboots
    // immediately into the previous verified image.
    void confirmInvalidAndReboot();

    // Begins receiving a new firmware image of the given total size
    // (bytes) into the inactive OTA slot. Returns false if there isn't
    // room, or an update is already in progress.
    bool begin(size_t image_size);

    // Writes the next chunk of firmware data. Call repeatedly as data
    // arrives (e.g. from an HTTP download) until the full image_size
    // from begin() has been written. Returns false on a write error
    // (caller should call abort() and retry from begin()).
    bool write(const uint8_t *data, size_t len);

    // Finalizes the update after all bytes have been written -
    // verifies the image and, on success, sets the new slot as the
    // boot target for next reboot (does NOT reboot itself - that's the
    // caller's call, e.g. after a graceful shutdown of other
    // subsystems). Returns false if the image was incomplete or failed
    // verification - nothing gets scheduled to boot differently in
    // that case, current firmware keeps running next reboot too.
    bool end();

    // Aborts an in-progress update (e.g. after a write() failure or a
    // download that was cancelled/timed out).
    void abort();

    // 0-100, meaningful only while an update is in progress (between
    // begin() and end()/abort()).
    uint8_t progressPercent() const;
};
