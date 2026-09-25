# Fix: `lib_extra_dirs` not taking effect in `platformio.ini` (native test env)

## Ground rules — read before touching anything

- Do **not** rename, delete, or rewrite the *content* of any file except the exact moves listed in this prompt. Everything else in this repo — all library code under `firmware/lib/`, all test files under `firmware/test/`, `firmware/platformio.ini`'s existing content, `firmware/src/main.cpp` — is already correct and has been hand-verified by compiling it directly with `g++`. This is a narrow, mechanical fix for one specific PlatformIO configuration mistake. It is not an invitation to "clean up," refactor, reorganize, or improve anything else you notice along the way.
- Do not touch anything under `docs/`.
- If this device's real commercial name appears anywhere in the codebase or docs (it shouldn't, by standing project policy), do not write it into any file, comment, or commit message. Always refer to it only as "a commercial industrial IoT gateway reference device" if it needs to be referenced at all.
- You cannot compile or run `pio` in your own environment. Do not claim anything is "tested," "verified," "working," or "passing" in your summary — say exactly what files you moved and nothing more. A human will run the actual `pio test -e native` afterward to confirm.
- If, while doing this, you find that any of the files/paths named below don't exist or don't match what's described, **stop and report that discrepancy instead of improvising a fix**. Don't guess.

## Background (for context only — don't act on this section, it's already done)

`firmware/test/native_stubs/` is a directory of hand-written fake headers (`HardwareSerial.h`, `Preferences.h`, `LittleFS.h`, `Arduino.h`, `Update.h`, `esp_ota_ops.h`, `esp_system.h`, `system_watchdog.h`) plus one fake implementation file (`fake_system_watchdog.cpp`). These exist so that `pio test -e native` (a host-native build with no real ESP32/Arduino framework available) can compile library source files that would otherwise need real Arduino/ESP-IDF headers. `firmware/platformio.ini`'s `[env:native]` section already has:

```ini
lib_ignore = system_watchdog
lib_extra_dirs = test/native_stubs
```

`lib_ignore` is confirmed working (verified by a real `pio test -e native` run — the real `lib/system_watchdog/` folder is successfully excluded). `lib_extra_dirs` is confirmed **not** working — none of the fake headers in `test/native_stubs/` are being found by PlatformIO's Library Dependency Finder (LDF) at all, even though the directory and files genuinely exist at that path.

## The actual bug

PlatformIO's `lib_extra_dirs` directive expects each listed path to be a **parent directory that itself contains one or more library folders** — exactly the same structure as the project's own `lib/` folder, where `lib/modbus_master/`, `lib/power_safety/`, etc. are each one library. It does **not** treat the listed directory itself as a single library.

Right now, `test/native_stubs/` has all 9 files sitting **directly inside it** (flat), with no further subfolder:

```
test/native_stubs/
  HardwareSerial.h
  Preferences.h
  LittleFS.h
  Arduino.h
  Update.h
  esp_ota_ops.h
  esp_system.h
  system_watchdog.h
  fake_system_watchdog.cpp
```

Because `lib_extra_dirs = test/native_stubs` tells PlatformIO "look inside `test/native_stubs/` for library folders," and there are none (just loose files), LDF finds nothing usable there and silently contributes zero include paths — which exactly matches the observed symptom (every one of those fake headers now reports "No such file or directory," including `system_watchdog.h`, which had been working correctly before this flat layout was introduced).

## The fix — a single directory nesting change, nothing else

Move all 9 files from `test/native_stubs/` into a **new subfolder** `test/native_stubs/arduino_fakes/`, so the structure becomes:

```
test/native_stubs/
  arduino_fakes/
    HardwareSerial.h
    Preferences.h
    LittleFS.h
    Arduino.h
    Update.h
    esp_ota_ops.h
    esp_system.h
    system_watchdog.h
    fake_system_watchdog.cpp
```

Do this as a plain move (preserve every file's content byte-for-byte — do not re-type, reformat, or "fix" anything inside them). The folder name `arduino_fakes` is not sacred — if you have a strong structural reason to name it differently, that's fine, but it must be a **single new subfolder directly under `test/native_stubs/`** containing all 9 files together (they must stay together in ONE folder, not split into multiple — several of them reference each other, e.g. `fake_system_watchdog.cpp` implements the class declared in `system_watchdog.h`, both need to resolve as the same library).

**`platformio.ini` needs no change at all.** `lib_extra_dirs = test/native_stubs` already correctly points at the *parent* directory — it was correct all along; the files living directly inside it (instead of inside a subfolder) was the actual mistake. Do not add, remove, or modify any other line in `platformio.ini`.

## After the move

List the exact `mv`/file operations you performed (old path → new path, for all 9 files) in your summary, and confirm nothing else in the repository was touched. Do not attempt to run `pio` yourself — a human will run `pio test -e native` afterward (possibly after also clearing PlatformIO's build cache with `pio run -t clean -e native`, which is a separate, independent troubleshooting step not part of this fix) to confirm all four currently-failing tests (`test_modbus_master`, `test_ota_update`, `test_power_safety`, `test_ota_session`) now build.
