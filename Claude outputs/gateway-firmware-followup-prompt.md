# Follow-up on the Priority 4/5 fixes — please clarify before I sign off

Thanks for the P4/P5 pass. Before I mark these done, I need answers on a few specific points — some are just documentation accuracy, one might be a real bug you found but didn't fully flag.

## 1. P4.1 — Ethernet reset pin: is GPIO9 actually referenced in code, or is this a docs-only note?

Your summary says "the code (GPIO9) and docs (GPIO38) mismatch has been documented." GPIO9 is `PIN_I2C_SCL` in `pin_map.h` — the shared I2C bus clock line. If `ethernet_link.cpp` (or anywhere else) actually passes or references GPIO9 for the Ethernet reset pin, that is not a documentation mismatch — that's a live pin collision between Ethernet reset and the I2C bus, and it needs to be treated as a real, higher-priority hardware conflict, not a docs note awaiting a hardware revision.

Please tell me exactly:
- The file and line where GPIO9 shows up in connection with Ethernet reset (if it does at all).
- Whether this GPIO9 reference is in actual firmware code, or only in a comment/doc you were reconciling.

If it turns out to be a real code-level reference to GPIO9 for Ethernet reset, do not change anything yet — just report back with the exact location so we can figure out how that got there before deciding what to do about it.

## 2. P4.5 — does the OTA rollback checkpoint still work for a freshly-commissioned device?

You moved `g_ota.confirmValid()` to run after a successful network connection, which is correct for the main case (a broken OTA image that can't reach the network now correctly stays unconfirmed and rolls back). But the case I need confirmed: what happens on a boot where the device has no configuration yet and goes straight into the commissioning portal instead of attempting a normal network connect? Does `confirmValid()` ever get called on that path, or does a freshly-commissioned device now never confirm?

Please check this specific path and either confirm it's already handled correctly, or fix it so entering the commissioning portal (a legitimate operational state on its own) also reaches the confirm checkpoint.

## 3. P5.2 — event log escaping: does it actually neutralize the field delimiter?

The original bug was about a **comma-separated** log format where an unescaped comma in a logged value could inject extra fields. Your summary title calls this "Event Log JSON Escaping" and describes escaping quotes and control characters (`\n`, `\r`, `\t`) — it doesn't mention commas.

Please confirm:
- Is the actual on-disk log format CSV (comma-separated) or JSON? (The original finding assumed CSV — if that's changed, say so.)
- If it's still CSV: does the escaping you added specifically handle a comma appearing inside a logged field, or only quotes/control characters? If commas aren't handled, that's the original vulnerability still open — please fix it to escape (or otherwise safely neutralize) the actual field delimiter, not just quote/control characters.

## 4. P4.7 — did you check the board manifest, or apply the flag defensively without checking?

The instruction was to check the `esp32-s3-devkitc-1` board manifest's actual default for USB-CDC-on-boot before making a change, and report which case it was (manifest already defaulted to CDC → no real conflict existed; manifest defaulted to physical UART0 → a real conflict existed and RELAY3/RELAY4 were actually colliding with the console). The fix you applied (`-D ARDUINO_USB_CDC_ON_BOOT=1`) is fine either way, but I want the documentation to reflect what was actually true, not an assumption.

Please tell me which case it was, and where in the manifest you confirmed it (or say plainly if you applied the flag without checking, so I know to verify it myself).

## 5. P4.3 (OTA anti-downgrade) — here's how to proceed

First, check whether this codebase already has a firmware-version identifier somewhere — a `FIRMWARE_VERSION` define, a build-time-injected string/number, anything currently used for logging or telemetry that identifies which build is running. Search before assuming there's nothing.

- If something like that already exists: use it as the basis for comparison against the OTA trigger's `version` field.
- If nothing exists: implement the simplest safe scheme — a monotonic `uint32_t` build number, stored in NVS alongside the rest of `device_config` (add a new field following the existing `device_config` getter/setter pattern), compared against the trigger's declared version with plain integer greater-than. Any trigger payload whose `version` field doesn't parse as a valid non-negative integer should be rejected outright — fail closed, consistent with how the rest of `ota_trigger` already handles malformed fields. Don't implement semver parsing or anything more elaborate than that; it's not needed here.

Add a test covering: reject equal version, reject lower version, accept strictly higher version, reject unparseable version field.

## 6. T1/T3 (Modbus and OTA native tests) — please don't add ArduinoFake, use the existing stub pattern instead

This codebase already has an established, working pattern for exactly this problem — hand-written stub headers matching the real third-party API surface, paired with an instrumented fake implementation, compiled and run as a native behavior test (this is how the LTE/PPP work and the commissioning-portal/device_config work were verified earlier in the project — see any existing `stub/`-style headers already in the test tree for the shape to follow). Bringing in a third-party mocking framework now would introduce a second, inconsistent testing style alongside the one already in use for no real benefit.

Please write minimal hand-rolled fake headers for the specific pieces `modbus_master` and `ota_update` actually call (a fake `HardwareSerial` covering just the read/write/available surface `modbus_master` uses, and a fake `Update` covering just `begin`/`write`/`end`/rollback-related calls `ota_update` uses) — matching the same style already used elsewhere in this project's test tree, not a general-purpose mock of the whole Arduino API surface. Scope each fake to only what's actually called; don't build out unused API surface.

---

Reply with the answers to 1–4 first (these are quick confirmations, no code needed for most of them except wherever you find an actual gap), then proceed with 5 and 6 as new implementation work. As before: if anything turns out not to apply once you look, say so rather than forcing a change.
