# Fix: real ESP32-S3 build fails at partition table generation (flash size mismatch)

## Ground rules — read before touching anything

- Touch only the single line specified below in `firmware/platformio.ini`. Do not modify anything else in this file or any other file.
- Never write the device's real commercial name into any file, comment, or commit message. Refer to it only as "a commercial industrial IoT gateway reference device" if it needs mentioning at all.
- You cannot compile or run `pio`. Do not claim anything is "tested," "verified," "working," or "fixed." State exactly what you changed. A human will run `pio run -e esp32-s3-devkitc-1` afterward to confirm — this build takes roughly 15-20 minutes the first time, so don't expect instant feedback.
- If `[env:esp32-s3-devkitc-1]` doesn't look the way it's described below, stop and report the discrepancy instead of improvising.

## Background (context only, don't act on this part)

`pio run -e esp32-s3-devkitc-1` compiled every single source file successfully — all of FreeRTOS, WiFi, BLE, mbedtls, and every file under `lib/` and `src/` — with zero errors in the actual code. The build only failed at the very last step, generating the partition table:

```
Partitions tables occupies 15.6MB of flash (16384000 bytes) which does not fit in configured flash size 8MB. Change the flash size in menuconfig under the 'Serial Flasher Config' menu.
*** [.pio/build/esp32-s3-devkitc-1/partitions.bin] Error 2
```

This is a configuration mismatch, not a code defect. The real hardware is an ESP32-S3-WROOM-1U-N16R8 module (16MB flash), but `board = esp32-s3-devkitc-1` in `firmware/platformio.ini` is a generic PlatformIO board profile that defaults to an 8MB (N8) flash variant. The project's `[env:esp32-s3-devkitc-1]` section already has a `board_build.flash_size = 16MB` override with a header comment explaining exactly this mismatch — but the build log shows the framework's internal sdkconfig merge step (labeled `*** Compile Arduino IDF libs for esp32-s3-devkitc-1 ***` in the log) explicitly replacing the correct 16MB setting back down to 8MB:

```
Replace: CONFIG_ESPTOOLPY_FLASHSIZE="16MB" with: CONFIG_ESPTOOLPY_FLASHSIZE="8MB"
```

So `board_build.flash_size` is present and correctly written, but isn't fully honored at that specific internal build stage for this board/framework combination — a known category of rough edge where a generic board profile's own baked-in flash size wins over a project's override during one particular merge step.

## The fix

In `firmware/platformio.ini`, inside the existing `[env:esp32-s3-devkitc-1]` section, find this line:

```ini
board_build.flash_size = 16MB
```

Add a new line directly after it:

```ini
board_upload.flash_size = 16MB
```

`board_upload.flash_size` is a separate PlatformIO directive from `board_build.flash_size` (one affects the esptool/upload-side flash size declaration, the other affects the CMake/IDF build-side config) — some pioarduino build stages read from the upload-side setting rather than the build-side one, which is the likely reason the existing override isn't fully taking effect. Setting both together is the standard workaround for this situation.

Do not change, remove, or reorder any other line in this section (`board_build.arduino.memory_type`, `board_build.partitions`, `custom_sdkconfig`, etc. all stay exactly as they are).

## After the change

Confirm the one line you added and its exact position in the file (immediately after `board_build.flash_size = 16MB`). Don't attempt to run `pio` yourself — a human will run `pio run -e esp32-s3-devkitc-1` afterward, and if this single addition doesn't fully resolve it, the next step would likely be clearing the build cache (`pio run -t clean -e esp32-s3-devkitc-1`) since PlatformIO can sometimes cache a stale sdkconfig merge from a prior failed build — but that's a separate troubleshooting step, not part of this fix, and shouldn't be done pre-emptively.
