# Follow-up round 3 — T1 was substituted, not completed; one more regression test; then stop and compile

## 1. T1 is still open — it's about `modbus_pdu`, not `modbus_master`

The `modbus_master` fake/test you built (`HardwareSerial` fake + injected FreeRTOS symbols + `addTarget` test) is useful, but it isn't T1. T1 is: `firmware/lib/modbus_pdu`'s read-request builder functions, and both response-parsing functions (bit-response, write-response), have zero test coverage. These are pure C functions operating on byte buffers — no FreeRTOS, no serial, no hardware dependency of any kind — so they belong directly in the existing native `test_modbus_pdu` target with **no fakes or mocks required at all**.

Please add tests there covering:
- Each of the read-request builder functions currently untested (check `modbus_pdu.c`/`.h` for which ones exist and aren't yet exercised in `test_modbus_pdu`).
- `modbus_pdu_parse_write_response()` — including the exact fix from P2.2 (now requires `pdu_len == 5` for the applicable function codes). Add a case with a truncated 2–4 byte response sharing the expected function code, asserting it's now correctly rejected, and a case with a full valid 5-byte response asserting it's accepted.
- The bit-response parsing function (read coils / read discrete inputs response) — valid response, and at least one malformed/truncated case.

This should be a small, self-contained addition since none of it needs the FreeRTOS/serial infrastructure you already built for `modbus_master`.

## 2. Add the missing P4.4 regression test to the `modbus_master` work you already did

The concurrency fix in P4.4 was: `addTarget()` rejects being called after `start()` has already run (`mutex_ != nullptr`). Please add a test case to what you already built for `modbus_master` asserting exactly this — call `start()`, then call `addTarget()` again, and assert it's rejected (not silently accepted, not crashing). This is the specific behavior P4.4 was fixing, so it should be the one directly tested, not just the happy-path "accepts a valid target before start" case.

## 3. After 1 and 2: stop adding new code and compile everything that's accumulated

At this point there are several rounds of changes — P4.3's `isOtaVersionAcceptable()` test, the `modbus_master` fakes/tests, the `ota_update`/`esp_ota_ops` fakes/tests, and now the two additions above — none of which have been compiled by anyone in this loop. Before doing anything further:

- If you have any way to run `pio test -e native` (or even just compile the new test files with a plain `g++ -std=gnu++17 -Wall -Wextra` against the native environment's include paths) in your own environment, do that now and fix whatever compiler errors turn up, rather than writing more uncompiled code on top.
- If you genuinely cannot compile in your sandbox, say so plainly and stop there — don't add anything further until a real compile pass happens. In that case, the next step is on Ozcan's end (running `pio test -e native` locally), not another round of code changes from you.

Report back which case it was, and if you did get a compile pass working, report the actual pass/fail results per test file — not just "written."
