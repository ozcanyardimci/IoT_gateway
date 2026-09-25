# Build roadmap

10-step plan, in order. Status updated as we go.

## 1. Repo + tooling setup — DONE

GitHub repo created and pushed (`IoT_gateway`). VS Code + PlatformIO + pioarduino + KiCad +
Wokwi extension installed. PlatformIO build confirmed working end to end.

## 2. Fixed chip-level constraints check — DONE

Locked in before any design work — these can't change later without a different MCU module:

- Module: ESP32-S3-WROOM-1U-N16R8 — 16MB Quad SPI flash, 8MB octal-mode PSRAM.
- GPIO26-32 permanently reserved (flash). GPIO33-37 permanently reserved (octal PSRAM).
- GPIO0/45/46/3 are strapping pins. GPIO39/42/47/21 have nonstandard reset behavior.
- Exactly 3 hardware UART controllers available.
- No built-in DAC peripheral — analog output needs PWM+filter or an external DAC chip,
  decided at step 6.

Full detail in `architecture.md` under "Fixed MCU constraints."

## 3. One-page block diagram — DONE

Mermaid diagram + bus assignment table + board-to-board header signal count, in
`architecture.md`.

## 4. Isolated subsystem prototyping — DONE

Simulation only (KiCad/ngspice for power and analog circuits). Wokwi isn't used going
forward — most real subsystems here (LTE, Ethernet, RS485, RS232) can't be simulated in it;
that firmware is written against datasheets and validated at Rev-A instead.

Each subsystem gets its own build plan under `docs/subsystems/` before work starts on it.
Order:

1. Power — `docs/subsystems/power.md` — **DONE**
2. Core compute (ESP32-S3 bring-up) — `docs/subsystems/core-compute.md` — **DONE**
3. Digital inputs (8x, opto-isolated) — `docs/subsystems/digital-inputs.md` — **DONE**
4. Relay outputs (4x) — `docs/subsystems/relay-outputs.md` — **DONE**
5. Status indication (I2C GPIO expander + LEDs) — `docs/subsystems/status-indication.md` —
   **DONE**
6. Analog I/O (input + output) — `docs/subsystems/analog-io.md` — **DONE**
7. RS485 (isolated) — `docs/subsystems/rs485.md` — **DONE**
8. RS232 — `docs/subsystems/rs232.md` — **DONE**
9. Ethernet (W5500) — `docs/subsystems/ethernet.md` — **DONE** through schematic capture
   (2026-09-13); ERC and BOM still pending, same as every other subsystem's open items
10. WiFi — `docs/subsystems/wifi.md` — **DONE** 2026-09-13. Small scope: the
    ESP32-S3-WROOM-1U's radio is already fixed (core-compute); this was just the external
    antenna path (J2 SMA jack -> EARTH, coax pigtail, gain-limited antenna) added to
    `core-compute.kicad_sch`. ERC and BOM's off-board items pending, same as everywhere.
11. LTE (Quectel EG915U-EU) — `docs/subsystems/lte.md` — **DONE** through schematic capture
    and BOM (2026-09-19). Most complex subsystem: own rail, own UART (via a level shifter),
    SIM interface, power-control circuit, own antenna path. Two findings from drafting it
    were resolved early: the rail (renamed `VBAT_LTE`) was retapped from ~3.3V to ~3.82V for
    real margin against Quectel's 3.3-4.3V spec (see `power.md`'s revision history), and
    `architecture.md`'s header line now lists `VBAT_LTE` separately from `3V3_LOGIC` — see
    `lte.md` decisions 2 and 8. The sheet symbol + 6 hierarchical pins were added to the
    `lteboard.kicad_sch` master sheet 2026-09-19 and checked pin-by-pin against
    `lte.kicad_sch`'s own labels (full netlist reconstruction, not hand-traced coordinates)
    — exact match, no defect. Two real ERC-style gaps found in that same pass (D1's 4th ESD
    channel unwired, two U4 no-connect flags missing) have since been fixed and re-verified.
    ERC and BOM's off-board items pending, same as every other subsystem's open items. The
    master sheet's 6 new pins aren't wired to anything yet: `3V3_LOGIC`/`VBAT_LTE` need the
    board-to-board connector (step 7, not started — every subsystem's cross-board rails are
    in the same state, not LTE-specific), and `LTE_PWRKEY`/`LTE_RESET`/`LTE_TXD`/`LTE_RXD`
    need real MCU GPIO/UART pins that don't exist yet on `core-compute.kicad_sch` (step 6,
    not started).

ERC is deferred to a single end-of-project pass across all subsystems, not a per-subsystem
pre-merge gate (Ozcan's explicit decision, confirmed 2026-09-10 — status-indication had
already opted into the same project-wide-pass approach independently); tracked per-subsystem
in `CLAUDE.md`'s open items, not blocking this roadmap's forward progress.

All 11 subsystems above are now schematic-complete, which closes this step. Immediately
after, both boards were brought into a single schematic project (`ioboard+lteboard`) to
make cross-board wiring for steps 6 and 7 directly checkable instead of reasoned about
from two closed projects — full detail in `docs/board-merge.md`. That project is now the
sole active one; `ioboard/` and `lteboard/` are frozen as of 2026-09-19.

## 5. Per-subsystem acceptance criteria — DONE FOR POWER, PER-SUBSYSTEM GOING FORWARD

Each subsystem's own plan now defines its pass/fail acceptance criteria as part of that
subsystem's build (see `docs/subsystems/power.md`, step 11).

## 6. Fine-grained resource planning — SUBSTANTIALLY DONE, 2 ITEMS OPEN

Exact pin/bus assignment across the full design, done in `ioboard+lteboard` (see step 4
and `docs/board-merge.md`), since the pins being assigned on `core-compute.kicad_sch`
serve subsystems on both physical boards. All 28 cross-subsystem signals (LTE x4, RS485
x2, RS232 x2, Ethernet x6, I2C x2, digital inputs x8, relay outputs x4) now have real
GPIOs assigned and wired on `core-compute.kicad_sch` — full table in
`docs/architecture.md`. The analog-output DAC-vs-PWM question is also decided: an
MCP4725 I2C DAC (see `docs/subsystems/analog-io.md`), riding the existing shared I2C bus
— no separate PWM/GPIO line needed, `architecture.md` updated to match.

Two items still open before this step is fully closed:

1. `firmware/platformio.ini` still declares `board = esp32-s3-devkitc-1` with no
   flash/PSRAM override — a generic N8 profile, not the real N16R8 module (16MB flash /
   8MB octal PSRAM). Needs a `board_build.flash_size`/`board_build.psram_type` override
   (or an N16R8-specific board definition).
2. Five of the eight digital-input pin assignments landed on pins this project's own
   `architecture.md` flags under "Fixed MCU constraints" as sensitive: `DI4_MCU` →
   GPIO3 and `DI7_MCU` → GPIO46 (both strapping pins), `DI5_MCU`/`DI6_MCU`/`DI8_MCU` →
   GPIO39/42/47 (nonstandard reset behavior). This wasn't re-checked against that
   constraints list when the assignment was made — worth a deliberate accept-or-reroute
   decision before calling pin planning final, not a silent pass.

## 7. Incremental consolidation — DONE

Firmware: git branches per subsystem, merged incrementally. Hardware: KiCad hierarchical
sub-schematics per subsystem — done per-board already, and both boards' hierarchies were
brought into one schematic project 2026-09-19 (`docs/board-merge.md`).

Board-to-board connector decision made 2026-09-22 — see `docs/architecture.md`'s
"Board-to-board header" section for the full two-connector pinout (J_PWR 2x5, J_SIG
2x12, Samtec TSW/SSW 2.54mm). Reference-design photos were reviewed first to check for a
precedent part; the reference doesn't stack its two boards the same way (likely
cable/harness-linked), so its headers didn't dictate a specific part, only confirmed
2.54mm THT pin headers are a reasonable, unexotic choice.

Connector symbols placed and wired per that pinout (4 symbols: J_PWR_IO/J_PWR_LTE,
J_SIG_IO/J_SIG_LTE), no-connect flags added to spare pins, `rs485.kicad_sch`'s `EARTH`
label promoted to `global_label` for consistency with the other three sheets that use
it, and a defect on `PS1`/`U13` (RK-0515S / R1SX-3.33.3-R isolated DC-DC modules) caught
and fixed during the ERC pass — not a label swap: both SnapEDA-imported symbols had
their return-side output pin (`-VOUT`) generically typed as plain `Output` instead of
the electrically-correct `Power output`/`Passive` split, which reads as two driven
outputs shorted together ("pins of type output and output are connected"). Fixed by
retyping each symbol's `+VOUT` to `Power output` (the genuinely driven pin) and `-VOUT`
to `Passive` (the return/reference side, not a second driven output).

ERC confirmed clean 2026-09-23 (Ozcan, full project pass). This closes step 7.

## 8. Rev-A prototype PCB spin — NOT STARTED

First point real physical hardware is needed. Since schematic capture now lives in one
merged project (`ioboard+lteboard`) instead of two, producing the two separate physical
boards' PCB files needs a deliberate approach at this step — see the open question at the
end of `docs/board-merge.md`.

## 9. Integration schematic + PCB layout — NOT STARTED

Schematic integration is effectively already done ahead of this step — see step 4 and
`docs/board-merge.md`. What remains here is the PCB layout half, and settling the
two-PCBs-from-one-project question noted at step 8.

## 10. Fabrication, bring-up, enclosure, iterate — NOT STARTED

## Firmware roadmap

Tracked separately from the 10 hardware steps above since much of it can be written and
unit-tested before Rev-A hardware exists. Scope modeled on a comparable commercial 4G IoT
gateway in this device class (MCU-based, not a Linux-SBC gateway).

### F1. Protocol/logic layer — DONE, 2026-09-23

Hardware-independent, real unit tests (host-native compile via `gcc`/`g++` directly, not
`pio test` - see `CLAUDE.md`'s Firmware F1 section for why). All 6 modules in
`firmware/lib/`: modbus_crc, modbus_pdu, modbus_rtu, modbus_tcp, mqtt_payload, io_state.

- Modbus RTU CRC16 — **DONE**, tested against independently-computed reference vectors.
- Modbus RTU frame builder/parser — **DONE**. Role: master (this gateway polls
  downstream field devices, it does not expose itself as a slave).
- Modbus TCP — **DONE** (needed alongside RTU; this gateway has Ethernet).
- MQTT payload construction — **DONE**.
- DI/AI/relay ↔ register mapping table — **DONE** (`io_state`).

### F2. Drivers — DONE, 2026-09-23

Write + compile-verify only — no physical hardware exists yet, so nothing here can be
functionally tested before Rev-A. All 10 compile-checked with `g++` (8 against a
hand-written Arduino/Wire/HardwareSerial stub; `ethernet_link`/`wifi_link` reasoned
against the real ESP32 core headers instead - see `CLAUDE.md`'s Firmware F2 section for
why those two weren't stubbed).

DI, relay outputs, analog input, analog output (DAC), I2C GPIO expander + status LEDs,
RS485 UART, RS232 UART, Ethernet (W5500), WiFi, LTE (Quectel AT commands).

### F3. Industrial-grade layer — DONE (write + compile/gcc-verify only), 2026-09-24

Write + compile-verify only.

- Config/commissioning (WiFi AP + local web UI) — **DONE** (`commissioning_portal` +
  `device_config`: WiFi AP with a single-page form covering WiFi/MQTT/device-ID/TLS
  settings, persisted to NVS)
- MQTT resilience (reconnect/backoff, store-and-forward when offline) — **DONE**
  (`mqtt_client_wrapper`, wraps knolleary/PubSubClient)
- Local rule engine / condition-based actions (independent of cloud connectivity) —
  **DONE** (`rule_engine`, real gcc-tested same as F1)
- OTA update with rollback — **DONE** (`ota_update`: wraps arduino-esp32's `Update`
  API and ESP-IDF's OTA-state calls; overrides the weak `verifyRollbackLater()`
  symbol so a fresh OTA image boots PENDING_VERIFY until the app explicitly confirms
  it, rather than arduino-esp32's default of silently auto-confirming every boot,
  which would defeat rollback entirely. `firmware/partitions_16mb_ota.csv` provides
  the two-OTA-slot + otadata partition layout this needs, sized for the real 16MB
  module. Pure progress-percentage math (`otaProgressPercent`) is gcc-tested, same
  discipline as the rest - 7 checks, all passing.)
- NTP time sync — **DONE** (`ntp_time`, built-in `configTime()`, no dependency)
- Persistent event/diagnostic logging — **DONE** (`event_log`, rotating text log on
  LittleFS, current file rotated to `prev.log` past a 32KB cap - rotation behavior
  actually run against a functional stub, not just compiled)
- MQTT TLS + certificate-based device authentication + secure credential storage —
  **DONE** (`mqtt_tls_config` wraps `NetworkClientSecure` - core-3.x's renamed
  `WiFiClientSecure` - with cert/key material sourced from `device_config`'s NVS
  storage; storage itself is only as secure as flash encryption makes it, which
  isn't enabled yet - see `docs/secure-boot.md`, stated honestly in
  `device_config`'s own header too, not just here)
- WireGuard VPN — **DONE** (`wireguard_link`, wraps esphome/wireguard's real API -
  same library/API as droscy/esp_wireguard, which is now just a redirect to the
  renamed repo). Earlier versions of this line said it needed a real build-
  integration redesign (a `components/` folder, an ESP-IDF/Arduino mixed build) -
  that was based on an incomplete read of the library and was corrected 2026-09-24
  (see `CLAUDE.md`'s Firmware F3 section): it's a plain `lib_deps` entry like every
  other module here. What's still open is the same first `pio run` every F3 module
  is waiting on, not a design problem specific to this one.
- Firewall (network interface packet filtering) — **PARTIALLY DONE, honestly scoped
  down.** `net_allowlist` is a pure application-layer outbound-host allowlist, not a
  real packet filter - ESP32 Arduino has no netfilter-equivalent framework to wrap
  without dropping to raw ESP-IDF lwIP access that `framework = arduino`'s own API
  surface doesn't expose - the same kind of low-level lwIP access `wireguard_link`'s
  underlying library uses internally (it vendors its own lwIP netif code rather than
  going through Arduino's API), not something an application-level module like this
  one can get to. Deliberately not called a "firewall" in the code itself for that
  reason. Real interface-level filtering would need that same kind of IDF-level
  work - not done, not started.
- LTE registration/signal-quality resilience — **DONE** (`lte_modem` extended with
  AT+CSQ signal quality + full AT+CREG registration state + a `poll()`
  recovery-reset loop, parsing logic split into pure-C `lte_at_parse` and gcc-tested
  against 13 vectors taken from Quectel's own AT command manual)
- Power-loss / brownout safe-state handling — **DONE** (`power_safety`: persists a
  consecutive-brownout streak in NVS and recommends a geometric startup delay for
  power-hungry subsystems; reactive only, by design - no dedicated power-fail/
  voltage-sense signal exists in this design to react proactively to, checked
  `architecture.md`/`power.md` directly rather than assumed)
- Secure boot / flash encryption (built into the S3 silicon, needs enabling) —
  **GUIDANCE WRITTEN, NOT ENABLED** (`docs/secure-boot.md`) - the irreversibility
  detail, the eFuse specifics, and the recommended sequencing (OTA-with-rollback
  proven first, Development Mode before Release Mode) are all written up now, but
  nothing in this project's firmware turns either feature on.

All F3 items are now written and gcc/compile-verified where that's meaningful
(see the OTA line above - it was actually finished before this correction pass but
this section hadn't caught up). What's genuinely still open, here and across the
whole firmware, is the same thing: a first real `pio run` of the esp32-s3-devkitc-1
env (nothing has been build-verified from this environment at all - needs Ozcan's VS
Code). `wireguard_link` no longer carries its own separate build-integration risk on
top of that - see the corrected line above. **Update 2026-09-25: done, see F4.1
below - this whole paragraph is now historical.**

Deferred, explicit decision: meter-reading protocol support on RS232 (separate protocol
stack from Modbus, not part of this build for now — revisit if a real need comes up).

### F4. System integration — DONE, real `pio run`/`pio test` build-verified 2026-09-25

Task scheduler, watchdog, full build tying every module above together.

**Architecture (Ozcan's sign-off, 2026-09-24, after real back-and-forth - not a
unilateral call):** hybrid, not a single cooperative loop. Modbus RTU polling runs on
its own dedicated FreeRTOS task (`modbus_master`), because its frame timing (a ~4ms
silent inter-byte gap at 9600 baud detects frame boundaries - Modbus_over_serial_line
spec) can't tolerate being stalled by a slow MQTT TLS handshake or an OTA flash write
happening elsewhere - both confirmed, not assumed, to genuinely block for hundreds of
ms to a few seconds. Everything else runs in `main.cpp`'s `loop()`, cooperative/
poll()-driven, the same pattern most modules were already written in.

Built so far:
- **`system_watchdog`** (new lib) - thin wrapper around ESP-IDF's real Task Watchdog
  Timer API (`esp_task_wdt_init`/`add`/`reset`, confirmed against IDF v5.3.1's actual
  header, not assumed from older ESP32 tutorials that still show the pre-IDF-5
  `(timeout_s, panic)` signature). Both the main loop and the Modbus task register
  and feed it independently - either one hanging reboots the device.
- **`modbus_master`** (new lib) - the RS485 RTU polling engine. Deliberately narrow
  scope, stated in its own header: RTU only (Modbus TCP master would need a
  socket-client transport that doesn't exist in this codebase yet - real follow-up
  work, not an oversight), read-holding-registers only. Target list starts **empty on
  purpose** - nothing in this project's docs defines what downstream field devices
  this gateway actually polls (no register map, no slave addresses anywhere), so
  inventing a device list would be a guess, not an engineering decision.
  `addTarget()`/mutex-protected `getReading()` are the real, working mechanism;
  populating it is a per-deployment decision for later. The inter-frame-gap timing
  function is pulled out as a pure, gcc-testable function
  (`modbusRtuInterFrameGapUs()`) the same way `ota_update.h`'s progress math is - 6
  checks against the spec's own formula, all passing (`firmware/test/test_modbus_master`).
  Both this and `system_watchdog` were also compile-verified with real `g++` against
  hand-written `HardwareSerial`/FreeRTOS/`esp_task_wdt` stub headers (zero warnings,
  `-Wall -Wextra`) - real verification, not just "should compile." A native behavior
  test for Modbus utilizing a fake `HardwareSerial.h` and FreeRTOS stubs was written
  and committed to `test_modbus_master.cpp` - confirmed passing 2026-09-25, see
  F4.1 below.
- **`main.cpp` rewritten** - wires together every F1-F3 module that has a complete,
  unambiguous config path: local I/O, Ethernet/WiFi connectivity (LTE is powered on
  and its registration/signal tracked for the status LED, but NOT used as a
  connectivity backhaul - `lte_modem.h` itself says PDP/PPP isn't implemented, so
  there's no way for it to serve as a network `Client` yet), NTP, MQTT (+TLS via
  `mqtt_tls_config`), `net_allowlist` gating the real outbound hosts before
  connecting, `rule_engine` (loaded with zero rules - same "real mechanism, no
  invented content" reasoning as the Modbus target list), `event_log`,
  `power_safety`'s startup-delay, OTA's boot-time rollback confirmation (safety-
  critical, wired regardless of the trigger-flow gap below), and the watchdog/Modbus
  task themselves.
- **WireGuard config storage + wiring** (2026-09-24, following the web-research pass
  below) - `DeviceConfig` gained real NVS-backed storage: `setWireguard()`/8 getters/
  `hasWireguardConfig()`, same per-accessor open/close pattern as the existing TLS
  material methods, keys named to fit NVS's 15-char cap (`wg_privkey`, `wg_pubkey`,
  `wg_psk`, `wg_addr`, `wg_netmask`, `wg_endpoint`, `wg_port`, `wg_keepal`).
  `commissioning_portal`'s form gained a WireGuard section (checkbox-gated, so it's
  opt-in per device) collecting all 8 fields, rejecting the save with HTTP 400 if
  `wg_enable` is checked but a required field (private key/peer public key/local
  address/peer endpoint) is blank. `main.cpp` now loads this config in `setup()` and,
  if present, starts the tunnel via `wireguard_link` - but only AFTER NTP sync
  succeeds, not just after the network comes up: ESPHome's own WireGuard component
  documentation (same underlying library) states the tunnel requires a synchronized
  system clock, found during the careful research pass below and treated as a real
  requirement, not an assumption. If NTP sync fails that boot, WireGuard is skipped
  and logged rather than started against a possibly-wrong clock. The peer endpoint is
  also added to `net_allowlist` before `connectNetwork()` runs, consistent with how
  MQTT's broker host is already gated. Compile-verified clean (`g++ -Wall -Wextra`,
  zero warnings) for both changed files against hand-written `Preferences.h` (for
  `device_config.cpp`) and `WString.h`/`Arduino.h`/`WiFi.h`/`WebServer.h` (for
  `commissioning_portal.cpp`) stubs - the same discipline as `system_watchdog`/
  `modbus_master`. `main.cpp` itself is too large to fully stub (pulls in nearly
  every hardware driver module) - a brace/paren/bracket balance check was run
  instead, and real verification stays deferred to the joint `pio run` pass below,
  same as every other F4 change.

- **OTA update-trigger flow + signature verification** (2026-09-24, second item
  from the web-research pass below - the security-critical one, so it got its own
  extra research pass first). Real research (not assumed) ruled out both of the
  "obvious" approaches before writing any code:
  - ESP-IDF's `CONFIG_SECURE_SIGNED_APPS_NO_SECURE_BOOT` (documented as verifying
    app signatures on OTA without hardware Secure Boot) turned out to be a Kconfig
    setting resolved into the ESP-IDF build - and this project's `framework =
    arduino` ships the Arduino core precompiled, where Kconfig changes have no
    effect at all (confirmed via a direct answer from a PlatformIO community
    expert, not assumed). pioarduino's fork *may* support a `custom_sdkconfig`
    override, but that could only be confirmed against a third-party
    auto-generated wiki with no worked example and its own "no examples ... for
    security-critical flags" caveat - not solid ground for this project's actual
    security boundary.
  - arduino-esp32's own `Update.installSignature()` hook (built on that same
    mechanism) has a real, apparently still-open GitHub issue
    (espressif/arduino-esp32#12422, filed March 2026): `Update::begin()`
    unconditionally calls `reset()` internally, silently discarding the installed
    signature before it's ever checked - exactly the call order `ota_update.cpp`
    already uses. Relying on it would mean trusting a check that silently never
    runs, worse than no check with the gap known.

  Built instead: application-level verification, entirely in this project's own
  code, using mbedtls directly (already linked into every arduino-esp32 build via
  TLS - nothing new pulled in). New libs, each compile-verified clean
  (`g++ -Wall -Wextra`, zero warnings) and, where they have real pure logic, gcc-
  tested with a standalone harness AND a Unity test file under `firmware/test/`:
  - **`ota_trigger`** - hand-rolled (not a general JSON library - none exists in
    this project yet, not worth adding for one fixed 5-field schema) parser for
    the MQTT command payload (`url`/`sha256`/`sig`/`size`/`version`). Fails closed
    on anything malformed - missing fields, wrong hex length, escape characters,
    oversized `size` (bounded against `partitions_16mb_ota.csv`'s real 4MB OTA
    slot size). 14 checks, all passing.
  - **`url_host`** - extracts a hostname from a URL that must start with exactly
    `"https://"` (case-sensitive, single-slash-rejecting), for the allowlist gate
    below. 11 checks, all passing.
  - **`image_verify`** - `Sha256Streaming` (mbedtls's real streaming SHA-256 API,
    not hand-rolled hashing) + `verifySignature()` (mbedtls ECDSA P-256 verify via
    `mbedtls_pk_verify`, chosen over RSA for its much shorter ~70-72 byte signature
    - meaningfully smaller for an MQTT-payload-constrained trigger command) +
    `base64Decode()` (mbedtls's real base64, not hand-rolled). Every mbedtls call
    verified against the EXACT commit ESP-IDF v5.3.1 vendors
    (`espressif/mbedtls@72aa687`, looked up directly via GitHub's API, not
    assumed) - not just "some mbedtls version". Fails closed on an empty/
    placeholder public key. Compile-verified against hand-written stub headers
    matching that exact API, PLUS a separate behavior test with fake mbedtls
    functions confirming the fail-closed control flow actually works (empty key
    rejected, parse failure rejected, bad signature rejected, valid signature
    accepted) - 10 checks, all passing.
  - **`ota_signing_key.h`** - the verification PUBLIC key, compiled into firmware
    source, deliberately NOT stored via `DeviceConfig`/NVS or commissioning-portal-
    configurable like WireGuard's keys or MQTT's TLS material - if it were, anyone
    able to commission this device (an unauthenticated local AP, per
    `commissioning_portal.h`'s own documented caveat) could install their own key
    and sign their own "updates". Ships as an empty placeholder so verification
    fails closed (rejects every trigger) until Ozcan actually generates and pastes
    in a real key pair - the file's own header has the exact `openssl` commands
    for both key generation and signing a release.
  - **`ota_session`** - orchestrates one attempt end-to-end: allowlist-gates the
    download host, applies `DeviceConfig`'s OTA CA cert to a DEDICATED
    `NetworkClientSecure` (not `g_mqtt_tls_client` - reusing that would disrupt
    MQTT's own persistent session), downloads via `HTTPClient`, streams each chunk
    into both `OtaUpdate::write()` (F3's existing rollback-capable OTA writer) and
    `Sha256Streaming`, and only calls `OtaUpdate::end()` (making the new image
    bootable) after the final hash matches the command's `sha256` AND
    `verifySignature()` passes - `OtaUpdate::abort()` on any earlier failure.
    Enforces its own 30s stall timeout independent of the watchdog feed inside the
    download loop, so a connection that stays open but stops delivering bytes
    can't hide behind "the watchdog's still being fed." Not gcc-testable itself
    (real HTTPS/flash I/O, no pure logic left after the three modules above
    already extracted everything that could be pulled out) - reviewed by hand
    against every called function's real signature instead, matching `main.cpp`'s
    own verification standard. A monotonic version downgrade-prevention check was
    extracted to a pure function (`isOtaVersionAcceptable()`) and tested explicitly
    in `test_ota_session.cpp`. In addition, native tests utilizing fake `Update.h`
    and `esp_ota_ops.h` stubs were written and committed to `test_ota_update.cpp` -
    confirmed passing 2026-09-25, see F4.1 below.
  - **A real design gap caught during this pass, not after**: `net_allowlist` is
    populated once at boot from known-in-advance hosts (MQTT broker, WireGuard
    peer). An OTA trigger's own `url` field is untrusted network input - if
    `ota_session` allowlist-checked THAT host, anyone able to publish to the MQTT
    command topic could point this device at any HTTPS host just by naming it in
    the payload, making the allowlist decorative. Fixed by adding a SEPARATE,
    pre-configured `DeviceConfig::setOtaHost()`/`getOtaHost()` (commissioning-form
    field, added to `net_allowlist` at boot the same way MQTT/WireGuard already
    are) - `ota_session` only ever allowlist-checks against that pre-approved
    host, never the trigger payload's own claim.
  - `mqtt_client_wrapper` gained `subscribe()`/`onMessage()` (PubSubClient
    supports both natively - confirmed against its real header, not assumed) with
    automatic re-subscribe after every reconnect (PubSubClient's `connect()`
    establishes a clean session each time - a subscription does NOT survive a
    disconnect/reconnect on its own without this). Receive buffer bumped to 700
    bytes (`MQTT_RX_BUFFER_SIZE`) to fit the trigger payload's worst case - the
    256-byte PubSubClient default is shared between send and receive, confirmed
    against its real header rather than assumed.
  - `main.cpp` subscribes to `"<device_id>/cmd/ota"`; the message callback parses
    the payload synchronously (fast/pure, no I/O) and just sets a pending flag -
    the actual download+flash-write is deferred to `loop()` (`runOtaSession()`),
    since that can legitimately take tens of seconds and has no business running
    inside a PubSubClient callback. `ota_update.h`'s boot-time rollback
    confirmation (independent of trigger mechanism, safety-critical) was already
    wired before this and is unchanged.

- **LTE backhaul via PPP** (2026-09-24, fourth/final item from the web-research
  pass below). Research found arduino-esp32 already ships a built-in `PPP` library
  (`PPP.h`, wrapping ESP-IDF's `esp_modem` component) directly in the 3.x core -
  no `lib_deps` entry needed, and a much smaller task than the originally-assumed
  "write a custom `esp_modem` device profile." Confirmed directly against that
  library's real source (`PPP.h`, `NetworkInterface.h`, `esp_modem_c_api_types.h`,
  all fetched from `espressif/arduino-esp32`/`espressif/esp-protocols` and read, not
  assumed):
  - **`CONFIG_LWIP_PPP_SUPPORT`** gates the whole library
    (`#if CONFIG_LWIP_PPP_SUPPORT && ARDUINO_HAS_ESP_MODEM`). Multiple arduino-esp32
    GitHub issues (#7483, #7203) confirm this was OFF by default at points in the
    2.x core's history; this project's precompiled `framework = arduino` core
    couldn't be checked further (no browsable prebuilt `sdkconfig` in
    `esp32-arduino-libs`'s repo). Mitigated, not blocked on: added
    `custom_sdkconfig = CONFIG_LWIP_PPP_SUPPORT=y` to `platformio.ini`
    (`custom_sdkconfig` itself confirmed real and working via pioarduino's own
    issue #335, a `CONFIG_BT_ENABLED=n` example - not assumed). Risk-calibrated
    differently from the OTA-signing Kconfig question above: if this assumption is
    wrong, the failure is a LOUD compile error (PPP symbols missing), not a silent
    security hole, so proceeding past it with a documented mitigation was judged an
    acceptable risk here where it wasn't there.
  - **`lte_modem`** trimmed to power/reset GPIO sequencing ONLY - its old UART/AT
    transport (`sendAt()`, `AT+CSQ`/`AT+CREG` polling, `LteRegistrationState`,
    `LtePollResult`) removed, since `PPPClass` needs exclusive ownership of the same
    UART0 peripheral to frame AT/PPP traffic correctly; two independent
    `HardwareSerial` users on one UART would corrupt each other's reads.
    `lte_at_parse.h/.c` (pure AT-response parsing, previously gcc-tested with 13
    vectors) was deleted as dead code once nothing called `sendAt()` anymore -
    confirmed via a repo-wide grep before deleting that it had no other dependents.
  - **New `lte_ppp` lib** wraps the global `PPP` singleton with a minimal
    `WifiLink`/`EthernetLink`-style API (`begin(apn, sim_pin)`, `isAttached()`,
    `switchToDataMode()`, `isConnected()`, `signalQualityDbm()`). Deliberately does
    NOT call `PPPClass::setResetPin()` even though it exists - `lte_modem` already
    owns PWRKEY/RESET (verified against the real schematic back in the original LTE
    power-sequencing work), and having two independent pieces of code drive the same
    reset line risked an unwanted reset mid-session. `PPPClass::begin()`'s
    `uart_num` parameter defaults to 1 - passing that default would have silently
    collided with `Rs485Serial`, which owns UART1; `lte_ppp` passes `0` explicitly
    (this modem's real UART, per `lte_modem.h`/`pin_map.h`). Model is
    `PPP_MODEM_GENERIC` - the Quectel EG915U isn't one of `PPPClass`'s named
    profiles (confirmed by reading `ppp_modem_model_t` directly: only
    GENERIC/SIM7600/SIM7070/SIM7000/BG96/SIM800, plus a CUSTOM slot this project
    hasn't set up), so GENERIC (esp_modem's standard 3GPP AT command set) is the
    best fit available through this API - untested against real hardware, stated
    plainly rather than presented as verified end to end. Connection sequence
    (`begin()` in command mode -> poll `attached()` -> `switchToDataMode()`
    (`ESP_MODEM_MODE_CMUX`) -> poll `isConnected()`) matches Espressif's own
    official `PPP_Basic.ino` example, fetched and read directly.
  - **`main.cpp`**: `connectNetwork()` gained LTE as a genuine third tier (only
    attempted if `DeviceConfig::hasLteApn()`), with the same bounded-wait/
    watchdog-feed shape Ethernet/WiFi already used, split into two waits (attach,
    then data mode) for `lte_ppp`'s two-stage sequence. `g_lte.begin()`/
    `powerOn()` moved to run immediately BEFORE `connectNetwork()` (previously
    after MQTT setup) - `lte_ppp`'s `begin()` needs the modem already powered up
    and past its boot settle time before it starts talking AT commands to it.
    `DeviceConfig` gained `setLteApn()`/`getLteApn()`/`hasLteApn()` and
    `setLteSimPin()`/`getLteSimPin()` (SIM PIN optional, same "absence/empty both
    mean not set" precedent as WireGuard's preshared key); `commissioning_portal`'s
    form gained matching fields, no checkbox gate needed (an APN present is itself
    what `connectNetwork()` checks). The status LED and `networkIsUp()` now read
    `lte_ppp`'s real `isConnected()` instead of the old AT+CREG-based
    `isRegistered()` - no periodic poll-interval throttle needed either, since
    `PPP.connected()` is a direct read of already-event-backed state, not an AT
    round-trip.
  - Compile-verified clean (`g++ -Wall -Wextra`, zero warnings) against hand-written
    stub headers matching the real `PPP.h`/`esp_modem_c_api_types.h`/
    `NetworkInterface.h` APIs, PLUS two behavior tests beyond compile-only checking:
    one exercising `lte_ppp`/`lte_modem` against fake `PPPClass` method bodies (20
    checks - null/empty APN rejection, the UART0-not-default-1 collision-avoidance
    fact specifically, SIM PIN pass-through/omission, CMUX mode switch, RSSI
    pass-through), and a second copy-pasting `connectNetwork()`'s real body
    verbatim into a standalone harness with a fake instantly-advancing clock and
    fake `EthernetLink`/`WifiLink`/`LtePpp`/`DeviceConfig`/watchdog/log, covering
    all 8 branch paths (no-config fall-through, `begin()` failure, attach timeout,
    switch-mode failure, connect timeout, full success, empty-vs-null SIM PIN, and
    confirming WiFi succeeding still short-circuits before LTE is ever touched) -
    17 checks, all passing. `DeviceConfig`'s new accessors and
    `commissioning_portal`'s new form-save path were also compile+behavior-verified
    (12 and a clean compile respectively) the same way WireGuard's fields were.

`main.cpp`'s header comment previously flagged LTE as the one thing NOT wired up
("Ethernet/WiFi are this build's only IP backhauls right now"); that's no longer
true as of this entry - LTE is a real third tier now, on the terms described above.
What's still explicitly NOT done, restated here rather than left implicit:
`connectNetwork()` only runs once, at boot - there's no live "a backhaul dropped
mid-operation, fail over automatically" monitoring in `loop()` for ANY of the three
tiers (not a gap this pass introduced - Ethernet/WiFi already had none). The real
reference device reportedly advertises instant automatic cellular failover as the
industry pattern for this class of gateway (see the network-priority note below) -
this build's strict-sequential-at-boot fallback is a real, stated fidelity gap
against that, not a hidden one.

Was **not yet verified** as of this entry (2026-09-24): an actual `pio run` of the
integrated build. Every piece above got the strongest verification possible from
this environment (gcc/g++ compile-checks against real or stubbed headers), but
`main.cpp` itself pulls in nearly every module at once and wasn't mock-compiled
against a full Arduino/ESP-IDF stub set - building that stub set would be a large
undertaking on its own and wouldn't replace the real `pio run` anyway.
**Update 2026-09-25: done, see F4.1 below - this paragraph and the one below it are
now historical, kept for the record of what was and wasn't known at the time.**

**F4 read DONE (write + compile/gcc-verify scope) as of this entry** - all 4
items from the "be really careful" research pass (WireGuard, OTA, LTE, network
priority) were complete, verified to the standard described throughout this section.
The one thing left for F4 to be FULLY done, per Ozcan's own standing call directly
below, was the real `pio run` build/link pass on his machine - see F4.1 below for
how that went.

**Network priority - RESOLVED, 2026-09-24 (Ozcan's explicit decision, asked via 2
separate questions rather than decided unilaterally):**
1. **Order: `Ethernet -> LTE -> WiFi`** (changed from the original `Ethernet ->
   WiFi -> LTE`). Matches the real reference device's documented wired-primary +
   cellular-failover pattern more closely, while keeping WiFi as a last-resort tier
   (not dropped entirely, which would've matched the reference device even more
   exactly) because it's genuinely useful for this project's own bench/dev work
   without Ethernet or a live SIM - a need the commercial reference device doesn't
   have to account for. `connectNetwork()` was restructured from 3 sequential
   early-returns to Ethernet's early-return followed by an LTE block that now
   FALLS THROUGH to the WiFi block on any failure (begin failure, attach timeout,
   switch-mode failure, or connect timeout) rather than ending the function - a
   real control-flow change, not just a copy-paste reorder, since LTE moved from
   "last tier, failure = give up" to "middle tier, failure = try the next one."
   Verified with a rewritten copy of the same copy-paste-verbatim standalone
   harness used for the original 3-tier version (see the LTE bullet above) - 16
   checks, including one specifically confirming LTE failing at each of its 4
   distinct failure points still reaches WiFi afterward, not just the happy path.
2. **Timing: KEPT boot-time-only**, did not add live failover monitoring.
   `connectNetwork()` still runs once, in `setup()` - a backhaul dropping
   mid-operation still needs a reboot to fail over, for all three tiers, same as
   before. Live monitoring (periodic health checks in `loop()`, switch-on-drop,
   fail-back-on-recovery decisions) was explicitly scoped OUT as a separately-sized
   follow-up, not folded into this pass - a meaningfully bigger task than a boot-
   time reorder, and not what was asked for here.

**Ozcan's call (2026-09-24): build verification (`pio run`) is deferred until F4 is
finished, then done together in his VS Code - not attempted piecemeal per F1-F3
module.** This doesn't change what "done" meant for F1-F3 above (write +
compile/gcc-verify only, stated plainly in each section's own header) - it just means
the actual `pio run` build/link pass, which needs Ozcan's machine anyway (this
environment can't reach the PlatformIO registry - see CLAUDE.md's Firmware F3
section), happens once against the complete integrated firmware rather than
incrementally. When F4 is finished, that's the trigger to tell Ozcan and do the
verification pass together - not before.

### F4.1 Real build verification — DONE, 2026-09-25

The `pio run` build/link pass deferred above is now done, on Ozcan's own machine
(WSL, real `pio` binary), together as planned. It took 4 iterations to reach a clean
build — each one a real bug that no amount of gcc/g++ stub-compiling could have
caught, since the bugs were specific to the real ESP32-S3 build path (pioarduino's
two-stage IDF+Arduino build, SCons's dependency resolution, PlatformIO's include-path
handling, and this project's own `lib_deps` history) rather than to any module's
internal logic:

1. **Duplicate `libsodium` install** — `platformio.ini` pinned it explicitly in
   `lib_deps` at the same time `wireguard`'s own `library.json` already declared it
   as a dependency, so PlatformIO installed two separate copies. Both copies' `port/`
   source folder (reached via a `srcFilter` path that escapes their own `srcDir`)
   mapped to the same unqualified SCons build directory, causing a
   `'.../port' already has a source directory` collision. Fixed by removing the
   redundant explicit pin — `diff -rq` confirmed the two copies were byte-identical,
   so nothing was lost.
2. **`pin_map.h` not found** for 12 files across `lib/` and `src/` — `firmware/include/`
   was never actually on the compiler's search path for library builds under this
   project's LDF configuration, only assumed to be. Fixed with a single
   `-I$PROJECT_INCLUDE_DIR` in `build_flags`, rather than copying the header into
   each affected library folder.
3. **3 `void`-to-`bool` conversion errors in `main.cpp`** — the boot sequence's
   `if (!g_X.begin()) { log error }` pattern, correct for genuinely-fallible
   I2C-based peripherals, had been mistakenly copy-pasted onto three GPIO-only calls
   with no possible failure path (`DigitalInputs::begin()`, `RelayOutputs::begin()`,
   `LteModem::powerOn()`, all correctly declared `void`). Fixed by removing the
   erroneous wrapper at just those three call sites, verified against each
   function's real implementation first, not just the compiler's error text.
4. **Undefined references to `MqttClientWrapper::MqttOutbox::*` at link time** —
   `mqtt_client_wrapper.h` had `#include "mqtt_outbox.h"` positioned inside
   `class MqttClientWrapper`'s own `private:` section, a leftover from before
   `mqtt_outbox` was split into its own library. That made the compiler treat
   `MqttOutbox` as a class nested inside `MqttClientWrapper` in that one file, while
   `mqtt_outbox.cpp` and the native test both correctly use it as the top-level class
   it actually is — two different mangled symbols for calls that were supposed to
   resolve to the same implementation. Native tests never caught this because they
   exercise `MqttOutbox` directly and never include `mqtt_client_wrapper.h` at all.
   Fixed by moving the `#include` out of the class body to the top of the file with
   the other includes.

After those four fixes, `pio run -e esp32-s3-devkitc-1` succeeded end to end —
linked `firmware.elf`, built `firmware.bin`/`firmware.factory.bin` (bootloader +
partition table + app image merged), RAM 21.8% (71.4KB/327.7KB), Flash 32.7%
(1.37MB/4.19MB against the build's reported budget — the real 16MB module has far
more headroom than that figure implies, see `board_upload.flash_size` above).

`pio test -e native` also ran for the first time against the real Unity/PlatformIO
test runner (not the ad-hoc `g++` compiles used to hand-verify each F1-F4 module as
it was written) — **132 of 132 test cases passed**, across all 15 native-testable
modules (`io_state`, all 5 Modbus layers, `mqtt_outbox`, `mqtt_payload`,
`net_allowlist`, `ota_session`, `ota_trigger`, `ota_update`, `power_safety`,
`rule_engine`, `url_host`). Two small include-path gaps surfaced here too —
`<cstdio>` needed explicitly for `snprintf` in `test_net_allowlist.cpp` and
`test_ota_trigger.cpp`, previously pulled in transitively rather than declared — real
findings from the real test runner, same category as the build fixes above, not
something the hand-verification passes could have caught.

This is the first real (non-gcc-stub) verification of the whole codebase — hardware
still not required for any of it. See F5 below for what still needs real hardware.

### F5. Rev-A bring-up — NOT STARTED

The only point F2-F4 get functionally verified against real hardware instead of just
compile-checked.

*   **USB CDC Flag Verification — RESOLVED, 2026-09-25.** `ARDUINO_USB_CDC_ON_BOOT=1`
    was set defensively without confirming the board manifest's actual default.
    Checked directly against Ozcan's real PlatformIO install: neither
    `esp32-s3-devkitc-1.json` (the board manifest) nor the generic `esp32s3` variant's
    `pins_arduino.h` define this macro at all — the real fallback, confirmed in the
    core's own `cores/esp32/HardwareSerial.h`, is `#ifndef ARDUINO_USB_CDC_ON_BOOT
    #define ARDUINO_USB_CDC_ON_BOOT 0`. Without this project's explicit override,
    `Serial` would default to UART0 instead of the native USB-C connector this board's
    schematic actually wires to the S3's native-USB pins (see the D+/D- wiring defect
    caught and fixed at roadmap step 6). The explicit `=1` is therefore correct and
    necessary, not a redundant guess — no code or config change needed, this was a
    verification-only item.
