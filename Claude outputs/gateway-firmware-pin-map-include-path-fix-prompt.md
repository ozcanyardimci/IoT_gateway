# Fix: `pio run -e esp32-s3-devkitc-1` fails with `pin_map.h: No such file or directory`

## Ground rules — read before touching anything

- Touch only the single addition specified below, in `firmware/platformio.ini`. Do not modify anything else in this file or any other file.
- Never write the device's real commercial name into any file, comment, or commit message. Refer to it only as "a commercial industrial IoT gateway reference device" if it needs mentioning at all.
- You cannot compile or run `pio`. Do not claim anything is "tested," "verified," "working," or "fixed." State exactly what you changed. A human will run `pio run -e esp32-s3-devkitc-1` afterward to confirm.
- If `[env:esp32-s3-devkitc-1]`'s `build_flags` block doesn't look the way it's described below, stop and report the discrepancy instead of improvising.

## Background (context only, don't act on this part)

`pio run -e esp32-s3-devkitc-1` fails while compiling this project's own `lib/` modules with:

```
lib/ads1115_adc/ads1115_adc.cpp:4:10: fatal error: pin_map.h: No such file or directory
    4 | #include "pin_map.h"
```

`firmware/include/pin_map.h` exists and is real project code (real pin assignments) — it is not missing, it is simply not on the compiler's include search path for `lib/*` builds under this project's current `platformio.ini`. This is confirmed to be a single, project-wide gap, not a one-off: a repo-wide grep shows the following files all include `pin_map.h` the same way, and every one of them will hit this identical error once the build reaches them:

```
lib/ads1115_adc/ads1115_adc.cpp
lib/di_driver/di_driver.cpp
lib/ethernet_link/ethernet_link.cpp
lib/ethernet_link/ethernet_link.h
lib/lte_modem/lte_modem.cpp
lib/lte_ppp/lte_ppp.cpp
lib/lte_ppp/lte_ppp.h
lib/mcp4725_dac/mcp4725_dac.cpp
lib/relay_driver/relay_driver.cpp
lib/rs232_serial/rs232_serial.cpp
lib/rs232_serial/rs232_serial.h
lib/rs485_serial/rs485_serial.cpp
lib/rs485_serial/rs485_serial.h
lib/status_leds/status_leds.cpp
lib/status_leds/status_leds.h
src/main.cpp
```

Do not copy `pin_map.h` into any of these folders — that would create 11+ duplicate copies of real hardware pin-assignment code that could silently drift out of sync with each other. The correct fix is to add the project's `include/` directory to the compiler's search path once, at the environment level, so every one of these files (and any other file that references it) resolves it the same way `src/main.cpp` was always intended to.

## The fix

In `firmware/platformio.ini`, inside the `[env:esp32-s3-devkitc-1]` section, find this `build_flags` block:

```ini
build_flags =
    -D ARDUINO_USB_CDC_ON_BOOT=1
    -DBOARD_HAS_PSRAM
```

Add one new line to it:

```ini
build_flags =
    -I$PROJECT_INCLUDE_DIR
    -D ARDUINO_USB_CDC_ON_BOOT=1
    -DBOARD_HAS_PSRAM
```

`$PROJECT_INCLUDE_DIR` is PlatformIO's own built-in variable for the project's `include/` folder (resolves to `firmware/include` here) — using it instead of a hand-written relative path (`-Iinclude`) avoids any ambiguity about what directory the path is relative to. This makes `include/` visible to every compilation unit in this environment — `lib/*` and `src/*` alike — not just the files listed above.

Do not change, remove, or reorder the two existing `-D` flags, and do not touch anything else in this section (`board_build.flash_size`, `board_upload.flash_size`, `lib_deps`, `custom_sdkconfig`, etc. all stay exactly as they are).

## After the change

Confirm the one line you added and its position in `build_flags` (first line, before the two existing `-D` flags — order among `build_flags` entries doesn't functionally matter, but keep it first for readability since it's a search-path flag, not a preprocessor define). Don't attempt to run `pio` yourself — a human will run `pio run -e esp32-s3-devkitc-1` afterward. Since this fixes an include-path gap that affects 11+ files at once, expect the build to get substantially further than before, but don't assume it will complete cleanly — later files may still surface separate, unrelated issues, which is normal for a build this size and not a sign this particular fix was wrong.
