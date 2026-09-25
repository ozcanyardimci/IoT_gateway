# Follow-up round 2 — one gap in P4.3, and a correction on T1/T3

## Resolved, no action needed

- **P4.1** — confirmed no real GPIO9/Ethernet conflict exists; the earlier summary was wrong. Nothing to do.
- **P4.5** — the `isPendingVerification()` guard before `g_portal.begin(g_config)` correctly covers the commissioning-portal path. Nothing to do.
- **P5.2** — RFC 4180-compliant CSV escaping (doubled quotes, commas wrapped in quotes) is the right fix. Nothing to do.
- **P4.7** — accepted as-is. The flag is safe regardless of the board manifest's actual default, so this doesn't need to block anything. Add one line to `docs/roadmap.md` (or wherever open items are tracked) noting: *"`ARDUINO_USB_CDC_ON_BOOT=1` was set defensively without confirming the board manifest's actual default — verify against the real manifest during F5 hardware bring-up, once a real `pio run` environment exists."* That's the only remaining action on this one, and it's low-priority.

## P4.3 — one real gap: the actual downgrade-rejection logic is untested

The parsing tests (unparseable string, missing version, negative number) are good, but they only test that malformed input is rejected — they don't test the actual security property this feature exists for: that a validly-formatted but *older-or-equal* version number gets rejected, and a genuinely newer one is accepted.

You noted this couldn't be tested cleanly without stubbing the rest of `ota_session`. Fix that by extracting the comparison itself into a small, pure, standalone function rather than leaving it inline as `cmd.version <= config.getFirmwareVersion()` inside `ota_session.cpp`. Something like:

```cpp
// in ota_session.h/.cpp, or wherever makes sense given the file's existing structure
bool isOtaVersionAcceptable(uint32_t declared_version, uint32_t current_version);
```

Have `ota_session.cpp` call this instead of inlining the comparison, and write a plain native test against the pure function directly — no stubbing of networking, `OtaUpdate`, or anything else required, since it's just comparing two integers. Cover: declared == current (reject), declared < current (reject), declared > current (accept), and the boundary right at `current_version` itself. This is the actual security-relevant behavior of P4.3, so it shouldn't be the one part left untested.

## T1/T3 — correction: the stub files you searched for don't exist in this repo, and that's expected

You're right that you found nothing — they were never committed. Those stub headers (`PPP.h`, fake `HardwareSerial`, etc.) were built in a separate, ephemeral scratch environment during earlier development, used to verify the LTE/PPP and other F4 code, and then never carried into the actual repository. This is a known, already-tracked gap (it's the same issue as documentation-debt item D4 from the earlier review: verification claims exist in `docs/roadmap.md`/`CLAUDE.md` with no corresponding committed test). So there's nothing for you to find — you'll be writing these from scratch, which also happens to close part of that documentation debt once you do.

Here's what to write, since there's no existing file to copy the shape from:

**Fake `HardwareSerial` header** (for `modbus_master`'s RS485 UART use) — scope it to exactly what `modbus_master.cpp` calls on its serial object, nothing more. At minimum this is likely: `available()`, `read()`, `write(const uint8_t*, size_t)` (and/or the single-byte `write(uint8_t)` overload if used), `flush()`. Back it with an in-memory byte buffer you can pre-load with fake RTU response bytes and drain, so a test can simulate "here's what the RS485 line returned" without any real hardware. Match the real `HardwareSerial` method signatures from the arduino-esp32 core exactly (return types, `const`-ness, parameter types) so the fake type-checks as a drop-in — don't invent a simplified signature.

**Fake `Update` header** (for `ota_update.cpp`) — scope it to exactly what `ota_update.cpp` calls: `begin(size_t)`, `write(const uint8_t*, size_t)`, `end(bool)`, plus whatever rollback-related calls (`esp_ota_ops.h` functions like `esp_ota_mark_app_valid_cancel_rollback()` / `esp_ota_get_state_partition()` or whatever `ota_update.cpp` actually calls — check the real file rather than guessing) it uses for the confirm/reject rollback path. Give the fake instrumented state (bytes written so far, whether `end()` was called with `true` or `false`, whether it was called at all) so tests can assert on it, the same style as the existing `ppp_fake.cpp`-style fakes used elsewhere in this project's history.

One constraint to flag honestly: neither of us can compile-verify these for you right now (you don't have `pio`/`g++` in your sandbox, and I don't have `pio` in mine either — the only real compiler check either of us has done this whole project has been hand-verified against real API docs, never a real `pio run`). Write these as carefully as you can against the real, documented Arduino-ESP32 API signatures, then flag clearly in your summary that these specific new files need a local `pio test -e native` run by Ozcan to confirm they actually compile and pass, since nobody in this loop has run that yet. Don't claim "all tests passing" — say "written, not yet compiled" so nothing gets misrepresented in the docs later.

## When done

Update `docs/roadmap.md` to reflect: P4.3's full closure (including the extracted pure-function test), and that T1/T3 native tests now exist as committed files (name them) pending Ozcan's local compile/run confirmation. Don't mark them "verified" in the docs until that local run has actually happened.
