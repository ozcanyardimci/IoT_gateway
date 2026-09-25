#pragma once
#include "device_config.h"

// Minimal WiFi-AP + local web UI for first-time/field commissioning -
// F3's "config/commissioning" item. Scope decided here (the roadmap
// only said "WiFi AP + local web UI", not which fields): a single page,
// one POST endpoint, covering exactly what DeviceConfig stores (WiFi
// station credentials, MQTT broker host/port/TLS toggle, device ID,
// and - only if TLS is checked - the three PEM blobs pasted in as
// text). No captive-portal DNS redirect, no styling beyond usable, no
// multi-page flow - a deliberately small first cut, not the ceiling on
// what this could become.
//
// Caller decides WHEN to start this (e.g. no stored WiFi credentials
// yet, or a physical "commission" button held at boot) - this class
// doesn't make that call itself. Typical use:
//
//   DeviceConfig config;
//   CommissioningPortal portal;
//   if (!config.hasWifiCredentials()) {
//       portal.begin(config);
//       while (!portal.configSaved()) { portal.handleClient(); delay(10); }
//       portal.stop();
//       ESP.restart(); // re-boot into normal station-mode operation
//   }
//
// The backing WebServer is a single static instance in the .cpp (this
// device only ever runs one commissioning session at a time) - don't
// instantiate more than one CommissioningPortal concurrently.
class CommissioningPortal {
public:
    // Starts the AP (SSID/password below) and the web server on it.
    // The AP password is intentionally fixed/simple (this is meant as a
    // first-boot-only, physically-local commissioning step, not a
    // permanent network) - reconsider if a deployment ever needs the AP
    // to stay up longer than a single commissioning session.
    bool begin(DeviceConfig &config);

    // Call every loop() pass while commissioning is active.
    void handleClient();

    // True once the web form has been successfully submitted and saved
    // to DeviceConfig - the caller's cue to stop() and reboot.
    bool configSaved() const { return saved_; }

    void stop();

    static const char *AP_SSID;
    static const char *AP_PASSWORD;

private:
    DeviceConfig *config_ = nullptr;
    bool saved_ = false;
};
