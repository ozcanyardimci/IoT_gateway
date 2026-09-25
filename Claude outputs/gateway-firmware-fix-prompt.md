# Fix request: DIY ESP32-S3 IoT gateway firmware — confirmed findings

## Context

This is a from-scratch DIY IoT gateway firmware project (ESP32-S3-WROOM-1U-N16R8, PlatformIO/pioarduino, `framework = arduino`), built by an EEE student as a hands-on learning project. It replicates the functional pattern of a commercial industrial IoT gateway (Ethernet/LTE/WiFi backhaul, Modbus RTU polling, MQTT telemetry, remote config, signed OTA updates). **Do not write the name of any specific commercial reference product into any git-tracked file (code, comments, docs, commit messages) — refer to it only as "the reference device" if you need to at all.** This is a standing project rule, not new.

Two independent review passes have now gone through the F1–F4 firmware (protocol/logic, drivers, industrial-grade layer, system integration) and produced a confirmed, deduplicated list of findings below. **Your job this time is to actually fix these**, following the codebase's existing conventions (it already has an established pattern for checked-and-logged init calls, bounds-checked accessors, and Unity tests under `firmware/test/` — match it, don't introduce a new style). Work through the list in priority order. For each fix:

- Make the minimal correct change — don't refactor unrelated code while you're in a file.
- If a fix naturally implies a test (most of these do), add or extend the Unity test in `firmware/test/`.
- If a finding turns out, on inspection, to not actually apply to the current code (something may have shifted since these were written), say so explicitly rather than silently skipping it or forcing an unnecessary change.
- After each priority group, give a short summary of exactly what changed (files + one line each) before moving to the next group.

One item (Priority 4, M7) needs you to actually check a fact before touching any code — see its note.

---

## Priority 1 — Boot-sequence integrity (do these together, same root cause)

**P1.1 — I2C bus is never initialized (Critical).**
Nothing in the firmware calls `Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL)`. Three drivers ride the shared I2C bus and depend on it: `pca9535` (via `status_leds`), `ads1115_adc`, `mcp4725_dac`. Without `Wire.begin()`, every I2C transaction from these drivers will fail on real hardware (status LEDs and analog I/O non-functional). Add the `Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL)` call in `main.cpp`'s `setup()`, before any of the three I2C-bus consumers' own `.begin()` calls.

**P1.2 — Almost no subsystem init return value is checked in `setup()` (Critical/systemic).**
`g_modbus.begin()` is checked and logged on failure — that's the existing correct pattern. Nearly everything else isn't: `g_di.begin()`, `g_relays.begin()`, `g_ai.begin()`, `g_ao.begin()`, `g_leds.begin()`, `g_log.begin()`, `g_lte.begin()`, `g_watchdog.begin()` all return a boolean signaling hardware health, and none of the return values are inspected. A real hardware fault at boot (bad solder joint, wrong I2C address, SPI wiring issue) currently fails completely silently. Fix: check every one of these the same way `g_modbus.begin()` already is — log a warning (or whatever the established severity convention is) on failure, and decide per-subsystem whether a failure should be fatal (e.g., watchdog failing to init is probably worth being loud about) or degraded-but-continue (e.g., DAC failing shouldn't stop the whole device from booting). Use your judgment per subsystem but be consistent with how `g_modbus.begin()`'s failure is currently handled.

**P1.3 — Modbus task-spawn failure is discarded.**
`g_modbus.start(&g_watchdog)` returns a boolean indicating whether the underlying `xTaskCreatePinnedToCore()` call succeeded, and the return value is silently discarded in `main.cpp`. If task creation fails, the whole Modbus polling engine silently never runs. Check and log this return value the same way as P1.2.

---

## Priority 2 — Security / data-integrity

**P2.1 — `url_host` hostname extraction can be bypassed via URL userinfo (High).**
`extractHttpsUrlHost()` in `firmware/lib/url_host` stops at the first `/`, `:`, or `?` after `https://`, but never special-cases `@` (the URL userinfo delimiter). Real HTTP client URL parsing (RFC 3986 order) splits the authority on `@` first, keeping only what follows the *last* `@` as host[:port]. A crafted URL like `https://trusted-host.example:@evil.com/malicious.bin` currently extracts `trusted-host.example` (passes the allowlist) while the actual HTTP client would connect to `evil.com`. Fix `extractHttpsUrlHost()` to find the authority segment (up to the first `/` or end-of-string), then within that segment find the *last* `@` and take everything after it as the host[:port] candidate before applying the existing `:`/`?` truncation logic. Add a test case covering exactly the `user:pass@host` and `host:@evil.com` shapes.

**P2.2 — `modbus_pdu_parse_write_response()` doesn't validate response length (High).**
Real write-acknowledgment responses (write single coil / write single register / write multiple registers) are always exactly 5 bytes (function code + 2-byte address + 2-byte value/quantity). The current check only requires `pdu_len >= 2` and doesn't verify the echoed address/value. Fix: require `pdu_len == 5` for the applicable function codes, and add a test asserting a truncated (2–4 byte) response with a matching function code is now correctly rejected.

**P2.3 — `device_config.cpp`'s `readString()` silently reports success with an emptied value on overflow (High).**
It calls `Preferences::getString(key, out, out_size)` and discards the return value, then returns `true` based only on `prefs.isKey(key)`. Real `Preferences::getString()` returns 0 (and leaves the pre-cleared buffer untouched, i.e. empty) if the stored value doesn't fit the buffer — so an over-length stored value currently comes back as "successfully read, value is empty string," indistinguishable from a genuinely-empty stored value. Fix: check `getString()`'s actual return value and have `readString()` return `false` (or otherwise signal truncation, matching whatever error-signaling convention the rest of `device_config.cpp` uses) when the read didn't fit. Also check whether any setter path (config setters, the commissioning-portal form handler) should enforce a max length matching the fixed-size buffers callers read into (`main.cpp`'s WiFi/WireGuard/LTE buffers etc.) — if there's an easy place to add that bound, do it; if it needs a broader buffer-size audit, just fix `readString()`'s truncation-detection and note the length-limit gap in your summary rather than guessing at the right limits.

---

## Priority 3 — Contract violations currently masked by a well-behaved caller

**P3.1 — `OtaUpdate::end()` calls `Update.end(true)`, skipping the real completeness check (High).**
`Update.end(true)` tells the underlying Arduino `Update` library to skip its own "was the declared size actually fully written" rejection, silently redefining the expected size to whatever was written so far. This contradicts `ota_update.h`'s own documented contract ("returns false if the image was incomplete"). It's currently masked because the one real caller (`ota_session`) independently tracks total bytes read and aborts before ever calling `end()` on a mismatch — but the `OtaUpdate` class itself doesn't honor its own contract. Fix: have `OtaUpdate` track the expected total size itself (it should already know this, since the caller presumably passes it to `begin()`) and call `Update.end(false)`, or explicitly compare bytes-written against the expected size before calling `end()` at all and return `false` without committing if they don't match. Don't just rely on the caller continuing to behave correctly.

---

## Priority 4 — Medium-severity items

**P4.1 — W5500 Ethernet has no hardware reset GPIO wired (Medium — likely a hardware/docs issue, not purely firmware).**
`ethernet_link.cpp` passes `-1` for the reset pin, and `pin_map.h` has no corresponding `#define`. `docs/architecture.md`'s design notes claim a reset line exists for this exact recovery purpose, but its own pin table only lists 5 Ethernet signals and its changelog's "28 signals total" doesn't match the table (27). This likely isn't fixable in firmware alone — flag it back to me rather than inventing a GPIO assignment. If you want to do something now: fix the doc inconsistency (27 vs. 28, and either remove the reset-line claim or mark it as a known Rev-A gap) so the docs stop contradicting themselves, and leave the actual GPIO decision for a hardware revision.

**P4.2 — `pca9535`'s output shadow isn't synced from the chip's real power-on-reset state (Medium).**
`output_shadow_` defaults to `0x0000`, but the PCA9535's real POR output register value is `0xFFFF`. `begin()` only does an address-ACK probe, never reads the chip's actual current output state. The one real caller (`status_leds`) happens to call `writeOutputs(0xFFFF)` before ever calling `writePin()`, masking this — but the driver is documented as generic/reusable and shouldn't rely on caller behavior. Fix: either have `begin()` read back the chip's actual output register into `output_shadow_` (if the PCA9535 supports reading its own output register state — check the datasheet register map), or make `writePin()` refuse to operate (return false / assert) until an initial `writeOutputs()` call has established a known shadow state. Pick whichever is cleaner given the actual chip's readable-register support.

**P4.3 — No anti-rollback/downgrade protection on OTA (Medium).**
The trigger payload's `version` field is informational-only; a validly-signed old image (including one with a since-fixed vulnerability) can be replayed indefinitely. Add a monotonic minimum-version check: store the currently-running firmware's version (however that's currently tracked — check if there's already a version constant/NVS field, don't invent a second one) and reject an OTA trigger whose declared version is not strictly greater, before even attempting the download. If there's genuinely no existing version-tracking mechanism to hook into, say so and propose the minimal one rather than half-implementing it.

**P4.4 — `modbus_master` has unprotected shared state and a missing mutex null-check (Medium).**
`targets_[]`, `target_count_`, `last_polled_millis_[]` are written by `addTarget()` with no locking, while read by the polling task (`runLoop()`) with no locking either — currently safe only because `addTarget()` isn't documented/expected to be called after `start()`. Add the same mutex protection already used for the readings cache around `addTarget()`'s writes (or explicitly assert/reject `addTarget()` calls after `start()` has run, whichever fits the intended usage better). Separately, `getReading()` calls `xSemaphoreTake()` without checking `mutex_` for null first — add a null-check that fails safe (return false / no data) if called before `begin()`.

**P4.5 — Boot-time OTA rollback confirmation happens too early (Medium).**
`g_ota.confirmValid()` (canceling the bootloader's pending-rollback state) runs unconditionally near the start of `setup()`, before networking/MQTT are ever tested — so the rollback safety net only catches a crash before `setup()`, not a new image that boots but fails to actually connect. Move the `confirmValid()` call to after a meaningful "this firmware actually works" checkpoint — e.g., after the first successful network connect and/or first successful MQTT connect — so a new image that boots but can never reach the network still rolls back on the next reset. Be careful not to move it so late that a device with no configured network (fresh out of commissioning) gets stuck never confirming — think through the commissioning-portal-active case specifically and handle it (confirm valid once the portal itself is up and serving, since that's a legitimate operational state too).

**P4.6 — MQTT offline outbox can deliver messages out of order (Medium).**
The outbox fills the first free slot on enqueue and flushes in slot order, stopping at the first failure. If an interior message permanently fails (e.g., oversized payload) while slots ahead of it drain and free up, newer messages backfill those freed slots and get sent before the still-stuck older one. Fix: flush should stop advancing past a genuinely failed (not just transiently unpublishable) message and not free/reuse a slot out of order — or, simpler, make the outbox a true ring buffer / linked list with strict FIFO semantics rather than free-slot reuse. Pick whichever fits the existing outbox's data structure with the least churn, and add a test that reproduces the interior-failure-then-backfill scenario and asserts ordering is preserved.

**P4.7 — `Serial`/UART0 vs. RELAY3/RELAY4 conflict — VERIFY BEFORE CHANGING ANYTHING.**
`pin_map.h` puts RELAY3/RELAY4 on GPIO43/44 (the module's default UART0 TX/RX), on the assumption that `Serial` resolves to native USB-CDC, not physical UART0, for this board. `platformio.ini` doesn't explicitly set `ARDUINO_USB_CDC_ON_BOOT` or `ARDUINO_USB_MODE`, so this depends entirely on the `esp32-s3-devkitc-1` board manifest's own default. **Before touching any code for this one: actually check the board manifest** (`boards/esp32-s3-devkitc-1.json` in the installed platform package, or the pioarduino/arduino-esp32 board definitions) for its default USB-CDC-on-boot setting. If the manifest already defaults to USB-CDC — this is a non-issue, just add an explicit `build_flags = -DARDUINO_USB_CDC_ON_BOOT=1` (or equivalent) to `platformio.ini` to make the assumption explicit and no longer dependent on an implicit board default, and note in your summary that no pin conflict exists. If it does NOT default to USB-CDC — this is a real conflict, and you'll need to either force USB-CDC mode explicitly via `platformio.ini` build flags, or move RELAY3/RELAY4 off GPIO43/44 in `pin_map.h` (flag this to me instead of silently reassigning hardware pins, since that's a hardware-layer decision). Report which case it was and what you found in the manifest — don't just assert one way or the other without having actually looked.

---

## Priority 5 — Low-severity, batch these whenever convenient

- **L1.** `power_safety`: when NVS fails to open, it currently defaults to *no* startup delay. Since a failing NVS open is itself a plausible brownout symptom, default to applying the delay (fail toward caution) instead.
- **L2.** `event_log`: tag/message fields are written unescaped into comma-separated log lines. Escape or otherwise neutralize embedded commas/newlines in logged fields (even though no current call site passes attacker-influenced content, make it safe for future ones).
- **L3.** `mqtt_client_wrapper`: `publish()`/`enqueue()` don't null-check `topic`/`payload` before use (unlike `subscribe()`, which does). Add the same null-checks for consistency.
- **L4.** `ota_trigger`'s numeric parser relies on `unsigned long` being 32 bits; on a 64-bit host build a crafted oversized value can avoid saturation and get silently truncated by a later narrowing cast, bypassing the intended bound. Parse into an explicit `uint32_t`-safe path (e.g. use `strtoul` then explicitly clamp/reject values `> UINT32_MAX` before the cast) so behavior doesn't depend on host `unsigned long` width. Add a test near the actual 32-bit boundary, not just an arbitrarily large value.
- **L5.** `ota_session`'s download loop doesn't cap a single read to the remaining declared image size, so a compliant server sending the exact declared size immediately followed by more coalesced data triggers an avoidable abort instead of a clean stop. Clamp each read to `min(buffer_size, remaining_expected_bytes)`.
- **L6.** `image_verify`: `mbedtls_sha256_starts`/`mbedtls_sha256_update` return codes aren't checked. Check them and fail closed (this is a robustness improvement — currently fails safe anyway via the later hash-mismatch check, but don't rely on that).
- **L7.** Buffer-size mismatch: `ota_trigger` accepts signatures up to 180 chars, but `ota_session` decodes into a 128-byte buffer. Align the two bounds (128 is almost certainly the correct one, given real ECDSA-P256 signatures are far shorter than either) so a syntactically-valid-per-the-parser value can't be rejected later by the decode step instead of the parser.
- **L8.** `StatusLed::HEARTBEAT` is defined but never driven. Either wire it up (e.g., blink it in `loop()` as a basic "firmware is alive" indicator) or remove the unused enum value — your call, note which you picked.
- **L9.** `McpAnalogOutput` (`g_ao`) is initialized but has no consumer anywhere (no `loop()` usage, no rule-engine action type targets it). If there's no near-term plan to use it, leave it initialized (harmless, DAC POR state is 0V) but add a one-line comment noting it's currently unused, so it doesn't look like an oversight later.

---

## Test coverage gaps (add these regardless of whether the related bug above gets fixed first)

- **T1.** `modbus_pdu`: several read-request builders and both response-parsing functions (bit-response, write-response) have zero test coverage. Add tests, especially covering P2.2's fix.
- **T2.** `modbus_rtu`/`modbus_tcp`/`io_state`: add tests for too-short frame, buffer-too-small, out-of-range channel index, and null-pointer argument cases.
- **T3.** `ota_update.cpp`'s actual logic (rollback query/confirm/reject, `begin`/`write`/`end`) has zero test coverage — only a pure progress-percentage helper is tested. Add tests, especially covering P3.1's fix.
- **T4.** `power_safety::begin()`, including its NVS-failure fallback path (L1), has zero test coverage.
- **T5.** `url_host` has no test for the `@`-userinfo URL shape — add as part of P2.1.
- **T6.** No test exercises `ota_trigger`'s numeric parser near the actual 32-bit boundary — add as part of L4.

## Documentation debt (fix after the code changes above, so the docs describe the post-fix state)

- **D1.** `docs/roadmap.md` / `CLAUDE.md` still describe the `platformio.ini` flash-size/PSRAM board override as "still open" — it's already present in the file. Update both.
- **D2.** Both docs have a stale section describing an older `lte_modem` (with AT-command/UART logic and a separate `lte_at_parse` module) that was later deleted — contradicts a later, correct section in the same docs. Remove or clearly mark the stale section as superseded.
- **D3.** Several "N checks, all passing" test-count claims (`ota_trigger`, `url_host`, `net_allowlist`, `rule_engine`) don't match the actual number of test cases in the corresponding `firmware/test/` file. Recount and correct after you've added the new tests above.
- **D4.** Three verification claims (`image_verify`, the LTE/PPP module pairing, and the network-priority fallback logic in `main.cpp`) describe passing behavior tests with specific check counts, but no corresponding test file exists in the repo — those tests were run in an ephemeral scratch environment and never committed. If you have time, recreate them as proper committed tests under `firmware/test/`; if not, at minimum correct the docs to stop claiming a reproducible test exists.
- **D5.** `docs/roadmap.md` and `CLAUDE.md` disagree on whether the digital-input strapping-pin question (GPIO3/GPIO46 usage) is still open or already resolved. Reconcile to one consistent status.

---

## When you're done

Give a final summary organized by priority group (P1–P5, tests, docs), one or two lines per item: what you changed, or why you determined a finding didn't apply. Flag anything from P1–P4 you couldn't fully resolve without a decision only I can make (P4.1's Ethernet reset GPIO and the "real GPIO conflict" branch of P4.7 are the two most likely candidates) rather than guessing.
