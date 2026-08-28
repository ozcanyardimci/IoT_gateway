# Build log

## 2026-08-28 — Project initialized
- Repository structure created (hardware/, firmware/, docs/).
- ESP32-S3 fixed pin/peripheral constraints confirmed (see docs/architecture.md).
- PlatformIO project scaffolded in firmware/, configured to use the pioarduino
  platform (github.com/pioarduino/platform-espressif32) instead of the stock
  PlatformIO Espressif32 platform, to track current Arduino-ESP32 core releases.
- Toolchain sanity check (blink sketch) added at firmware/src/main.cpp — to be
  built and flashed before any real subsystem firmware work starts.
