#pragma once

// WIZnet W5500 over SPI, via the ESP32 Arduino core's own native ETH.h -
// not a third-party PlatformIO library, so no extra package-registry
// fetch needed at build time (verified against Espressif's own
// arduino-esp32 example, ETH_W5500_Arduino_SPI.ino, 2026-09-23).
//
// Pins per pin_map.h / architecture.md: CS=GPIO10, MOSI=GPIO11,
// SCLK=GPIO12, MISO=GPIO13, INT=GPIO14. This board's W5500 wiring has no
// separate RESET pin (architecture.md's Ethernet row lists only
// CS/MOSI/SCLK/MISO/INT) - passed as -1 (unused) to ETH.begin(), the same
// pattern that example uses for boards without one.
//
// NOT verified against your actual schematic the way the LTE power pins
// were - I confirmed the pins exist in architecture.md's table, but
// didn't trace ethernet.kicad_sch net-by-net. Worth a look before relying
// on it if Ethernet gives you trouble.
class EthernetLink {
public:
    bool begin();
    bool isConnected() const;
};
