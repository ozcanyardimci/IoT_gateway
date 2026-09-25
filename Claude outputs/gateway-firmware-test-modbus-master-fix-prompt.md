# Fix: `test_modbus_master` still fails under `pio test -e native`

## Ground rules — read before touching anything

- Same rules as the previous fix prompt: touch only the exact files listed below. Do not rename, delete, or rewrite the content of anything else. All other library code, all other tests, and `docs/` are already correct and hand-verified — leave them alone.
- Never write the device's real commercial name into any file, comment, or commit message. Refer to it only as "a commercial industrial IoT gateway reference device" if it needs mentioning at all.
- You cannot compile or run `pio`. Do not claim anything is "tested," "verified," "working," or "passing." State exactly what you changed. A human will run `pio test -e native` afterward to confirm.
- If any file/path below doesn't match what's described, stop and report the discrepancy instead of improvising.

## Background (context only, don't act on this part)

`test_modbus_master` still fails to build under `pio test -e native`, on two new errors that only appeared once an earlier, unrelated fix (moving fake headers into `test/native_stubs/arduino_fakes/`) cleared a different blocker in front of them:

```
lib/rs485_serial/rs485_serial.cpp:2:10: fatal error: pin_map.h: No such file or directory
lib/modbus_master/modbus_master.cpp:5:10: fatal error: freertos/FreeRTOS.h: No such file or directory
```

Both are the exact same underlying issue this project has hit repeatedly: a library `.cpp` file (compiled by PlatformIO as its own build step) needs a header that currently only exists somewhere NOT visible to it — either a real header that isn't on the native build's search path, or a fake stub that's only visible to one specific test's own private folder. The fix pattern is always the same: put a copy where `test/native_stubs/arduino_fakes/` (already wired into `[env:native]` via `lib_extra_dirs = test/native_stubs` in `platformio.ini` — do not touch that line, it's already correct) makes it visible to every native compilation unit, not just one test's own files.

## Step 1 — copy `pin_map.h` into the shared stub folder

`firmware/include/pin_map.h` already exists and is real project code (real pin assignments) — do not edit its content. Copy it (don't move/delete the original — it's needed for the real `esp32-s3-devkitc-1` build too) to:

```
firmware/test/native_stubs/arduino_fakes/pin_map.h
```

Exact same content, byte-for-byte.

## Step 2 — move the FreeRTOS stub folder into the shared location

`firmware/test/test_modbus_master/freertos/` currently contains three files: `FreeRTOS.h`, `task.h`, `semphr.h`. These are native-test fakes for FreeRTOS, not real code — move (not copy) the whole `freertos/` folder into the shared stub location, so it becomes:

```
firmware/test/native_stubs/arduino_fakes/freertos/FreeRTOS.h
firmware/test/native_stubs/arduino_fakes/freertos/task.h
firmware/test/native_stubs/arduino_fakes/freertos/semphr.h
```

Preserve their content exactly. After the move, `firmware/test/test_modbus_master/freertos/` should no longer exist.

`firmware/test/test_modbus_master/test_modbus_master.cpp` includes these via `#include <freertos/FreeRTOS.h>` etc. (angle brackets) — this will now resolve through the shared location instead of the old local one. No change needed to that file's include lines.

## Step 3 — consolidate the Arduino.h / Serial1 / millis / micros situation

This is the part that needs care, so read it fully before acting.

There are currently **two different `Arduino.h` files** that matter for this one test binary:

1. `firmware/test/native_stubs/arduino_fakes/Arduino.h` — the shared one (added for a different test's sake), currently contains only `millis()`.
2. `firmware/test/test_modbus_master/Arduino.h` — a test-local one containing `HardwareSerial Serial1(1);` (the actual global instance definition) plus `extern "C"` declarations for `millis()`, `micros()`, `delay()`.

`firmware/lib/modbus_master/modbus_master.cpp` (a library file) does `#include <Arduino.h>` (angle brackets) and calls both `millis()` **and** `micros()` — confirmed by grep, it uses both. Under real `pio test`, this angle-bracket include resolves through `lib_extra_dirs`, i.e. file 1 above — which is missing `micros()` entirely, so this would fail to compile even after steps 1–2.

Meanwhile `firmware/test/test_modbus_master/test_modbus_master.cpp` itself never includes `Arduino.h` directly at all — it just directly **defines** `unsigned long millis() { return 0; }` and `unsigned long micros() { return 0; }` as bare functions, and uses `Serial1.fake_reset()`, relying on `Serial1` being declared `extern` inside `HardwareSerial.h` (already shared, already fine) and defined somewhere else.

Do this, precisely:

**3a.** Edit `firmware/test/native_stubs/arduino_fakes/Arduino.h` to match this exact content (replace the whole file):

```cpp
#pragma once
#include <stdint.h>
#include "HardwareSerial.h"

// Shared native-test fake for Arduino.h. Declares (does not define) the
// three free functions modbus_master.cpp and other library sources call
// (millis/micros/delay), plus the global Serial1 instance every
// HardwareSerial consumer expects to exist - matching the real
// arduino-esp32 core's own globals. Bodies are defined per-test (in
// whichever test_*.cpp actually needs specific timing behavior), with
// matching extern "C" linkage, so this header can be shared across every
// native test without forcing one fixed implementation on all of them.

extern HardwareSerial Serial1;

#ifdef __cplusplus
extern "C" {
#endif
unsigned long millis();
unsigned long micros();
void delay(uint32_t ms);
#ifdef __cplusplus
}
#endif
```

**3b.** Edit `firmware/test/test_modbus_master/test_modbus_master.cpp`: add `#include <Arduino.h>` to its include block near the top (alongside its existing `#include "modbus_master.h"` / `#include "HardwareSerial.h"` / `freertos/*.h` includes), so this test's own translation unit sees the exact same `extern "C"` declarations that `modbus_master.cpp` sees. Do **not** remove or change its existing `unsigned long millis() { return 0; }` / `unsigned long micros() { return 0; }` definitions — those bodies stay exactly as they are; they now correctly satisfy the shared header's `extern "C"` declarations instead of floating undeclared. Also add a matching `void delay(uint32_t ms) {}` definition (a plain no-op body) right next to the existing `millis()`/`micros()` bodies, since the shared header now declares it and something must define it in this test binary.

**3c.** `test_modbus_master.cpp` still needs `Serial1` to actually be *defined* somewhere (not just declared `extern`), since it calls `Serial1.fake_reset()`. Add this one line to `test_modbus_master.cpp`, near its other global fake state (wherever makes sense given the file's existing structure — near the top, after the includes, is fine):

```cpp
HardwareSerial Serial1(1);
```

**3d.** Delete `firmware/test/test_modbus_master/Arduino.h` — its content is now fully absorbed into the shared header (declarations) plus `test_modbus_master.cpp` (definitions), so the old file is redundant. Leaving it in place risks two different `Serial1`/`millis` definitions existing at once.

## After all three steps

List every file you moved, copied, edited, or deleted, with old path → new path where relevant. Don't run or claim to have run `pio test` — say plainly that this needs a real `pio test -e native` run to confirm, same as before.
