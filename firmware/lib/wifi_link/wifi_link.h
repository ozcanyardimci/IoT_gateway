#pragma once

// Thin wrapper around the ESP32-S3's native WiFi radio (station mode,
// which is all core-compute.kicad_sch wires up - antenna only, no
// external pins per architecture.md's bus table). Connect/status/
// disconnect only - reconnect backoff and AP-mode commissioning belong to
// F3 (industrial layer), not here.
class WifiLink {
public:
    void begin(const char *ssid, const char *password);
    bool isConnected() const;
    void disconnect();
};
