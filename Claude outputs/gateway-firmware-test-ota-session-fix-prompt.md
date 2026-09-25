# Fix: `test_ota_session` fails under `pio test -e native`

## Ground rules — read before touching anything

- Touch only the exact files listed below. Everything else in the repo — all other library code, all other tests, `docs/`, `platformio.ini`'s existing content — is already correct and hand-verified. Do not rename, delete, or rewrite the content of anything not explicitly named here.
- Never write the device's real commercial name into any file, comment, or commit message. Refer to it only as "a commercial industrial IoT gateway reference device" if it needs mentioning at all.
- You cannot compile or run `pio`. Do not claim anything is "tested," "verified," "working," or "passing." State exactly what you changed. A human will run `pio test -e native` afterward to confirm.
- If any file/path below doesn't match what's described, stop and report the discrepancy instead of improvising.

## Background (context only, don't act on this part)

`test_ota_session` fails to build under `pio test -e native` with two errors:

```
lib/image_verify/image_verify.h:4:10: fatal error: mbedtls/sha256.h: No such file or directory
lib/ota_session/ota_session.h:7:10: fatal error: NetworkClientSecure.h: No such file or directory
```

Both come from PlatformIO's Library Dependency Finder pulling in the *entire* `lib/ota_session/` folder (and, transitively, `lib/image_verify/`) as soon as `test/test_ota_session/test_ota_session.cpp` references `ota_session.h` — even though that test only actually exercises one small pure function, `isOtaVersionAcceptable()`. Everything else in `ota_session.cpp` (`runOtaSession()`) does real HTTPS I/O and flash writes and is explicitly documented in its own header comment as "deliberately NOT unit-testable... reviewed by hand" — it was never meant to be pulled into the native test build at all.

Unlike the previous fakes this project has written for other tests, faking `NetworkClientSecure`, `HTTPClient`, and real SHA-256/ECDSA crypto (`mbedtls`) well enough to actually exercise `runOtaSession()` would be a large, risky undertaking for something nothing currently tests. The correct fix here is not a fake — it's the same kind of physical split already done for `mqtt_outbox` (which was pulled out of `mqtt_client_wrapper` for the identical reason: a small, dependency-free piece stuck inside a folder that also needs a heavy third-party API). `isOtaVersionAcceptable()` has zero dependency on HTTPClient/NetworkClientSecure/mbedtls — it's a two-line `uint32_t` comparison — so pulling it into its own tiny library removes the need to fake any of those three APIs at all.

Confirmed before writing this prompt: `isOtaVersionAcceptable()` is called from exactly one place in the whole repo (`runOtaSession()`, inside `ota_session.cpp` itself) besides the test. `src/main.cpp` never calls it directly — it only calls `runOtaSession()`, which will still work unchanged after this split.

## Step 1 — create the new library

Create `firmware/lib/ota_version_check/ota_version_check.h`:

```cpp
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
```

Create `firmware/lib/ota_version_check/ota_version_check.cpp`:

```cpp
#include "ota_version_check.h"

bool isOtaVersionAcceptable(uint32_t declared_version, uint32_t current_version) {
    return declared_version > current_version;
}
```

## Step 2 — remove it from `ota_session`, wire up the new dependency

In `firmware/lib/ota_session/ota_session.h`:
- Remove this line (the declaration now lives in the new header):
  ```cpp
  bool isOtaVersionAcceptable(uint32_t declared_version, uint32_t current_version);
  ```
- Add `#include "ota_version_check.h"` near the top, alongside the existing includes (`"ota_trigger.h"`, `"device_config.h"`, etc.), so anything that includes `ota_session.h` still transitively sees the declaration.

In `firmware/lib/ota_session/ota_session.cpp`:
- Add `#include "ota_version_check.h"` near the top, alongside its existing includes.
- Remove the function body (do not change anything else in this file):
  ```cpp
  bool isOtaVersionAcceptable(uint32_t declared_version, uint32_t current_version) {
      return declared_version > current_version;
  }
  ```
- Leave the call site at line 43 (`if (!isOtaVersionAcceptable(cmd.version, current_version)) {`) exactly as it is — it will now resolve against the new header's declaration instead.

## Step 3 — point the test at the new, small header

In `firmware/test/test_ota_session/test_ota_session.cpp`, change:
```cpp
#include "ota_session.h"
```
to:
```cpp
#include "ota_version_check.h"
```
Nothing else in this file needs to change — it only ever called `isOtaVersionAcceptable()`.

## Step 4 — delete the now-unnecessary stub

Delete `firmware/test/test_ota_session/NetworkClientSecure.h`. It was a placeholder stub (`class NetworkClientSecure {};`) added only so `ota_session.h`'s declarations could parse for this test — after step 3, this test no longer includes `ota_session.h` at all, so the stub has nothing left to support.

## After all four steps

List every file you created, edited, or deleted, with a one-line note per file of what changed. Don't run or claim to have run `pio test` — say plainly that this needs a real `pio test -e native` run to confirm. Do not touch `platformio.ini` — this fix needs no configuration change, only the file moves/edits above.
