# Review request: DIY ESP32-S3 IoT gateway firmware (F1–F4)

## Who you are talking to and what this document is

You are being asked to perform an independent, detailed, **read-only** review of a firmware codebase. This document gives you the full context you need: what the project is, why it exists, how it's built, what's been verified so far, and a specific list of findings from a prior review pass that need independent verification.

**Do not modify, fix, refactor, or "clean up" any code or documentation in this repository.** Your job is to review, verify, and report — point out anything wrong, missing, inconsistent, or worth flagging, in as much detail as you can, but leave every file exactly as you found it. The person running this review will decide what to fix and do it themselves (or ask for that separately). If you believe something needs a code change, describe the change in your report; do not make it.

---

## 1. Project purpose

This is a from-scratch DIY IoT gateway project — both PCB hardware design (KiCad) and firmware (PlatformIO/ESP32-S3) — built by an electrical & electronics engineering (EEE) student as a hands-on learning project covering hardware and firmware development end to end. The student has prior hands-on experience with STM32/PIC microcontrollers, KiCad PCB design, industrial protocols (Modbus, MQTT-based IoT gateways), and completed a gateway-focused internship at an industrial-IoT company before starting this project.

The functional target is to replicate the capabilities of a real commercial industrial IoT gateway product (multi-backhaul connectivity — Ethernet/WiFi/cellular — with Modbus RTU polling, MQTT telemetry, remote configuration, and OTA updates). The specific commercial product being used as a functional reference is intentionally not named in this document or anywhere in the repository — treat the "reference device" mentions below as "a comparable commercial industrial gateway product," not a specific brand to look up or reproduce trademarked material from. Where a design decision below says "matches the reference device's pattern," that's describing an architecture pattern (e.g. "wired-primary with cellular failover"), not copying any proprietary implementation detail.

## 2. Hardware overview

Target MCU module: **ESP32-S3-WROOM-1U-N16R8** — 16MB Quad SPI flash, 8MB octal-mode PSRAM. Board profile in the build system is a generic "esp32-s3-devkitc-1" (N8) profile with flash/PSRAM explicitly overridden to match the real N16R8 module (see platformio.ini below).

Subsystems, with their real GPIO assignments (`firmware/include/pin_map.h`):

- **Shared I2C bus** (GPIO8 SDA / GPIO9 SCL) — carries three devices:
  - ADS1115 4-channel ADC (I2C addr 0x48) — analog input
  - MCP4725 12-bit DAC (I2C addr 0x60) — analog output
  - PCA9535PW 16-bit I2C GPIO expander (I2C addr 0x20) — drives the status LEDs
- **Ethernet**: WIZnet W5500 over SPI (CS=GPIO10, MOSI=GPIO11, SCLK=GPIO12, MISO=GPIO13, INT=GPIO14). No hardware reset pin currently wired to the MCU (flagged as a gap below — a subsystem doc says this was meant to have one).
- **LTE modem**: Quectel EG915U-EU, UART + control lines (TXD=GPIO17, RXD=GPIO18, PWRKEY=GPIO15, RESET=GPIO16, both power/reset lines via NPN low-side driver transistors, active-high at the MCU GPIO / active-low at the modem pin — this polarity was independently confirmed against the modem's hardware design doc, the real KiCad schematic, and circuit theory).
- **4x relay outputs** (GPIO7, GPIO38, GPIO43, GPIO44 — the last two share the module's default UART0 TX/RX pins, which is stated as safe because UART0 is otherwise unused; see gap list item about this).
- **RS232** (MAX3232EIPWR level shifter, TXD=GPIO5, RXD=GPIO6).
- **RS485** (isolated transceiver, TXD=GPIO1, RXD=GPIO2) — used for Modbus RTU master polling of downstream field devices.
- **8x digital inputs**, opto-isolated, active-low (GPIO40, 41, 48, 3, 39, 42, 46, 47 — two of these, GPIO3 and GPIO46, are ESP32-S3 strapping pins; this was reviewed and accepted as safe for this design, not overlooked).

Hardware development status: schematic-level design is done for the subsystems above (Rev-A). No PCB has been fabricated yet — there is no physical board to test firmware against. All firmware verification so far has been software-only (see Section 5).

## 3. Firmware architecture

- **Build system**: PlatformIO with the pioarduino fork of the ESP32 platform, `framework = arduino` (i.e. running on top of the arduino-esp32 3.x core, which itself wraps ESP-IDF — the exact ESP-IDF version this core pins is v5.3.1, confirmed by inspecting arduino-esp32's own version-lock files during development).
- **Partition table**: custom `partitions_16mb_ota.csv` — dual OTA app slots (`app0`/`app1`, 4MB each) + otadata + nvs + spiffs + coredump, sized for the real 16MB flash (the stock 8MB-board default partition table was wrong for this module and was overridden).
- **Two build environments** in `platformio.ini`:
  - `[env:esp32-s3-devkitc-1]` — the real target build.
  - `[env:native]` — host-native g++ build (`-std=gnu++17 -Wall -Wextra`) for hardware-independent pure-logic modules, run via `pio test`.
- **Library dependencies** (`lib_deps`, pinned to GitHub URLs rather than the PlatformIO registry, because the development environment could not reach the PlatformIO registry but could reach github.com directly):
  - `knolleary/pubsubclient` (MQTT client)
  - `esphome-libs/wireguard` v0.4.6 (WireGuard tunnel — this is the modern name for the library formerly published as `droscy/esp_wireguard`)
  - `esphome-libs/libsodium` v1.10021.11 (WireGuard's own crypto dependency)
- **LTE/cellular backhaul** uses arduino-esp32's own **built-in `PPP` library** (`PPP.h`, wrapping ESP-IDF's `esp_modem` component) — no separate `lib_deps` entry needed, it ships inside the arduino-esp32 3.x core itself, gated behind the `CONFIG_LWIP_PPP_SUPPORT` ESP-IDF Kconfig option. Because this project's `framework = arduino` build uses a **precompiled** Arduino core (Kconfig options normally have no effect against a precompiled core), `platformio.ini` adds an explicit `custom_sdkconfig = CONFIG_LWIP_PPP_SUPPORT=y` override — pioarduino's fork is confirmed (via its own issue tracker) to support this override mechanism for precisely this situation.
- **No real `pio run` build has been performed yet.** This is explicitly called out below (Section 5) — it is the single biggest unverified assumption across the whole codebase.

### Project phase structure (as tracked in the repo's own `docs/roadmap.md`)

- **F1 — Protocol/logic layer**: pure, hardware-independent protocol code (Modbus PDU encoding/decoding, Modbus RTU/TCP framing, Modbus CRC16, MQTT JSON payload construction, shared I/O state struct). Unit-tested via the `[env:native]` build.
- **F2 — Drivers**: thin hardware wrapper classes for every subsystem in Section 2 (I2C devices, SPI Ethernet, UART LTE/RS485/RS232, digital I/O).
- **F3 — Industrial-grade layer**: NVS-backed device configuration storage, a WiFi-AP + web-form commissioning portal, MQTT-over-TLS client config, a simple condition→relay rule engine, a rotating diagnostic event log, NTP time sync, OTA firmware update with bootloader rollback support, a startup brownout/power-safety delay, boot reset-cause classification, and an outbound-host allowlist (a defense-in-depth boundary meant to stop firmware from connecting to arbitrary hosts).
- **F4 — System integration**: ties everything together in `main.cpp`. Adds a real-time task watchdog, a dedicated FreeRTOS task for Modbus RTU polling (separated from the main loop because Modbus's inter-byte frame-gap timing can't tolerate being stalled by a slow TLS handshake or flash write elsewhere), a WireGuard tunnel, an MQTT-triggered OTA update flow with cryptographic signature verification (ECDSA-P256 via mbedtls) on top of the OTA/rollback mechanism from F3, and cellular (LTE/PPP) backhaul as a third connectivity tier alongside Ethernet and WiFi.
- **F5 — Rev-A hardware bring-up**: not started. This is where F2–F4 get verified against real hardware for the first time, once a PCB exists.

### Networking / backhaul priority

`main.cpp`'s `connectNetwork()` tries, in order: **Ethernet → LTE → WiFi**, each with a bounded wait (not indefinite). This ordering — LTE ahead of WiFi — was a deliberate choice: it mirrors the "wired-primary with cellular failover" pattern of the commercial reference device, while keeping WiFi as a last-resort tier (rather than removing it) because it's genuinely useful for bench/development testing without an Ethernet cable or an active SIM. **This is boot-time-only** — `connectNetwork()` runs once in `setup()`; there is no live "backhaul dropped mid-operation, switch automatically" monitoring in the main loop for any of the three tiers. This is a known, explicitly-scoped limitation, not an oversight — see the gap list.

## 4. Repository layout

```
firmware/
  platformio.ini              # build config (see Section 3)
  partitions_16mb_ota.csv     # custom OTA-capable partition table
  include/pin_map.h           # single source of truth for every GPIO assignment
  src/main.cpp                # F4 integration — the ~650-line file wiring everything together
  lib/                        # one directory per module (see inventory below)
  test/                       # Unity test directories, one per pure-logic module that has tests
docs/
  roadmap.md                  # dated, detailed project log — what was built, decided, and why
  architecture.md             # hardware pin-assignment table and cross-subsystem wiring decisions
  subsystems/*.md             # per-subsystem hardware design notes
CLAUDE.md                     # a parallel, more decision-log-flavored project history file
```

`docs/roadmap.md` and `CLAUDE.md` between them contain a detailed, dated history of what was built, what was researched (with sources), what was decided and why, and what's still open. They are useful background but **were themselves found to contain some now-stale claims** during the review that produced the gap list below — see the "Documentation debt" section of the gap list. Don't take either doc's claims about current code state at face value; verify against the actual files.

### Full module inventory (`firmware/lib/`)

**F1 — protocol/logic (pure, no hardware dependency):**
| Module | Purpose |
|---|---|
| `modbus_pdu` | Modbus PDU (function-code payload) encode/decode — read coils, read discrete inputs, read holding/input registers, write single/multiple registers |
| `modbus_crc` | Modbus RTU CRC16 checksum |
| `modbus_rtu` | RTU frame wrapping (address + PDU + CRC) |
| `modbus_tcp` | Modbus TCP / MBAP header framing |
| `mqtt_payload` | Builds the JSON telemetry payload published over MQTT |
| `io_state` | Shared struct holding current DI/AI/relay/AO state, with bounds-checked accessors |

**F2 — drivers (hardware wrappers):**
| Module | Purpose |
|---|---|
| `di_driver` | Reads the 8 digital inputs |
| `relay_driver` | Drives the 4 relay outputs |
| `ads1115_adc` | ADS1115 I2C ADC driver |
| `mcp4725_dac` | MCP4725 I2C DAC driver |
| `status_leds` | Status LED control via the PCA9535 I2C expander |
| `pca9535` | Generic PCA9535 16-bit I2C GPIO expander driver (used by `status_leds`) |
| `wifi_link` | WiFi station-mode connect/status wrapper |
| `ethernet_link` | W5500 Ethernet connect/status wrapper |
| `lte_modem` | LTE modem power-on/power-off/hard-reset GPIO sequencing (PWRKEY/RESET only — **no longer** owns the modem's UART, see F4's `lte_ppp`) |
| `rs485_serial` | RS485 UART wrapper |
| `rs232_serial` | RS232 UART wrapper |

**F3 — industrial-grade layer:**
| Module | Purpose |
|---|---|
| `device_config` | NVS (`Preferences`)-backed persistent config: WiFi creds, MQTT broker, TLS material, WireGuard config, OTA host/CA cert, LTE APN/SIM PIN |
| `commissioning_portal` | WiFi-AP + web form for first-time device setup, writes into `device_config` |
| `mqtt_tls_config` | TLS client setup for the MQTT connection (`NetworkClientSecure`) |
| `mqtt_client_wrapper` | Wraps `PubSubClient` — connect/reconnect, subscribe, publish, an outbox queue for offline buffering |
| `rule_engine` | Simple condition (DI/AI state) → action (relay output) rule evaluation, bounds-checked against `io_state`'s real channel counts |
| `event_log` | Rotating diagnostic log (timestamp, level, tag, message), persisted to flash |
| `ntp_time` | NTP time sync wrapper |
| `ota_update` | OTA firmware update writer + bootloader rollback confirmation (`Update` library + `esp_ota_ops.h`) |
| `power_safety` | Startup brownout-detection backoff delay |
| `reset_reason` | Classifies the boot reset cause |
| `net_allowlist` | Outbound-host allowlist — fails closed by design; every outbound connection (MQTT broker, NTP, WireGuard peer, OTA download host) is meant to be checked against it before dialing out |

**F4 — system integration:**
| Module | Purpose |
|---|---|
| `system_watchdog` | Wraps ESP-IDF's Task Watchdog Timer; both the main loop task and the Modbus task register and feed it |
| `modbus_master` | RS485 Modbus RTU master polling engine, runs on its own dedicated FreeRTOS task, mutex-protects the shared readings cache |
| `wireguard_link` | Wraps the `esphome-libs/wireguard` library for a WireGuard tunnel |
| `ota_trigger` | Hand-rolled parser for an MQTT-delivered OTA command payload (`url`/`sha256`/`sig`/`size`/`version` fields) — untrusted network input, meant to fail closed on anything malformed |
| `ota_session` | Orchestrates one OTA attempt end-to-end: allowlist-gates the download host, downloads over HTTPS, streams into both the OTA writer and a SHA-256 hasher, only finalizes if hash + signature both check out |
| `image_verify` | SHA-256 streaming hash + ECDSA-P256 signature verification via mbedtls — the actual cryptographic gate for OTA image authenticity |
| `url_host` | Extracts a hostname from a `https://`-prefixed URL string, for allowlist checking |
| `lte_ppp` | Wraps arduino-esp32's built-in `PPP`/`PPPClass` for cellular data backhaul — owns the LTE modem's UART (which `lte_modem` no longer does) |

`firmware/src/main.cpp` is the integration point: it constructs one global instance of nearly every module above, does the full boot sequence (local I/O init, commissioning-portal fallback if unconfigured, config load, allowlist population, network connect, NTP sync, WireGuard start, MQTT connect + OTA-topic subscribe, Modbus task start), and runs the cooperative main loop (`loop()`) that polls/services everything except Modbus (which runs on its own task).

## 5. Verification status — what has and hasn't been checked

**No physical hardware exists yet**, and **no real `pio run` build/link pass has been performed**. Given that, verification so far has consisted of:

1. Careful, source-checked research before writing anything non-trivial — API signatures, register maps, and protocol details were checked against real datasheets/library source/RFC text (via web fetches during development), not written from memory/assumption. Several of these citations are recorded in the header comments of the files themselves and in `docs/roadmap.md`/`CLAUDE.md`.
2. Compiling every individual module against **hand-written stub headers** that reproduce the real third-party API signatures (Arduino core functions, `Preferences`, `PubSubClient`, `Update`, mbedtls, `PPPClass`, etc.), using a real host `g++` with `-Wall -Wextra`, to at least confirm each module type-checks cleanly against the real API shape.
3. For some of the more logic-heavy or security-critical modules, **behavior tests** beyond pure compilation — fake/mock implementations of the external dependency functions, driving the module's real code through specific scenarios and asserting on the outcome (e.g., confirming a malformed input is rejected, confirming a specific GPIO/UART number is passed through correctly).
4. `firmware/test/` contains Unity test suites for most (not all) of the F1–F3 pure-logic modules, runnable via the `[env:native]` PlatformIO environment.

**What this does NOT cover, and should be treated as unverified:**
- Whether the whole integrated firmware actually compiles and links as one program (`main.cpp` alone pulls in nearly every module at once; no one has attempted to stub that entire dependency graph — it was judged not worth building a stub set that large when a real `pio run` will be done anyway).
- Any real-hardware behavior at all — timing, electrical characteristics, actual I2C/SPI/UART bus behavior, actual RF/cellular behavior.
- Some of the F4-era behavior tests (see the "Documentation debt" section of the gap list below) were run in a temporary scratch environment and their results are **not preserved as committed, reproducible test files** in `firmware/test/` — meaning their "N checks passing" claims in the docs cannot currently be re-run or independently confirmed by inspecting the repo alone.

## 6. What you're being asked to do

A prior review pass (by another AI reviewer, working directly against this same repository) already went through F1–F4 module by module and produced the finding list in Section 7 below. **Your job is to independently verify each of these findings against the actual current repository**, and additionally look for anything the prior pass may have missed. Concretely:

1. For each finding below, open the actual file(s) referenced and confirm (or refute) the claim. State clearly whether you agree it's a real issue, and if you disagree, explain exactly why with reference to the actual code.
2. Assess severity/impact in your own words — don't just repeat the label given below.
3. Actively look for anything NOT on this list: this list should not be treated as exhaustive. Pay particular attention to: thread-safety between the Modbus FreeRTOS task and the main loop, any other place besides the ones listed where untrusted network input (MQTT payloads, OTA trigger fields, HTTP responses) flows into a fixed-size buffer or a security decision, and any other place besides `readString()`/I2C-init where a failure return value is silently discarded.
4. Report back in detail, findings-first. **Do not edit, patch, or "helpfully fix" anything** — this is an analysis pass only; the person you're reporting to will decide what to do with your findings and asked specifically that you not make changes.

---

## 7. Findings from the prior review pass (to independently verify)

### Critical

**C1. I2C bus is never initialized — `Wire.begin()` does not appear anywhere in the firmware tree.**
`main.cpp` calls `g_leds.begin()`, `g_ai.begin()`, `g_ao.begin()` (the PCA9535-backed status LEDs, the ADS1115 ADC, and the MCP4725 DAC — all three ride the shared I2C bus on GPIO8/GPIO9). None of those three drivers' `.cpp` files, and nothing in `main.cpp`, ever calls `Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL)` to actually attach the `TwoWire` driver to those pins and bring the I2C peripheral up. On ESP32 Arduino, `Wire.beginTransmission()` does not implicitly initialize the bus. As written, every I2C transaction on real hardware should fail: all 6 status LEDs, both analog channels, non-functional at boot. Compounding this: `main.cpp` never checks the return values of `g_leds.begin()`/`g_ai.begin()`/`g_ao.begin()` (unlike `g_modbus.begin()`, which is checked and logged), so this failure would be completely silent on a real device — no error, no log line, just non-functional LEDs and analog I/O.

### High

**H1. `url_host`'s hostname extraction can diverge from the host actually connected to (allowlist-bypass risk).**
`extractHttpsUrlHost()` scans left-to-right from just after `"https://"` and stops at the first of `/`, `:`, `?`, or end-of-string — it never special-cases `@` (URL userinfo delimiter). `HTTPClient`'s real URL parsing (used by `ota_session` to actually open the download connection) follows RFC 3986 order: split off the authority up to the first `/`, then split THAT on `@` (keeping only what's after the last `@` as host:port), then split on `:` for the port. For a URL like `https://trusted-host.example:@evil.com/malicious.bin`, `extractHttpsUrlHost()` returns `"trusted-host.example"` (stops at the `:` before ever considering `@`) — which would pass the allowlist check if that host is allowlisted — while `HTTPClient` actually connects to `evil.com`. This means the OTA download host allowlist (the layer specifically built to stop an attacker who can publish to the MQTT command topic from pointing the device at an arbitrary download server) can potentially be bypassed via a crafted trigger URL. Whether this is fully exploitable end-to-end also depends on the configured OTA CA certificate's trust scope.

**H2. `device_config.cpp`'s `readString()` silently reports success with an emptied value when a stored field doesn't fit the caller's buffer.**
It calls `Preferences::getString(key, out, out_size)` but discards the return value, then returns `true` (its "success" signal) based only on whether the key exists in NVS — not on whether the read actually fit. Arduino's real `Preferences::getString()` does not truncate on overflow; it returns 0 and leaves the destination buffer untouched, which here has already been pre-cleared to `""`. Net effect: if a stored value (WiFi password, WireGuard private/public/preshared key, LTE APN, etc.) is longer than the fixed-size stack buffer a caller reads it into, the caller gets back `true` with an empty string, with no way to distinguish "genuinely stored empty" from "silently truncated to nothing." Nothing on the write path (neither `device_config`'s setters, nor the commissioning-portal web form) enforces any length limit matching the various fixed-size read-side buffers scattered through `main.cpp`, so an over-length value is trivially reachable through normal commissioning.

**H3. `OtaUpdate::end()` calls `Update.end(true)`, which skips the underlying library's own completeness check.**
Arduino's real `UpdateClass::end(bool evenIfRemaining)`: when `evenIfRemaining` is true, it skips the "was the declared size actually fully written" rejection and instead silently redefines the expected size to whatever was actually written so far, then proceeds to verify and commit that as if it were the whole image. This contradicts `ota_update.h`'s own documented contract ("Returns false if the image was incomplete... nothing gets scheduled to boot differently in that case"). Currently masked in practice because the one real caller (`ota_session`) independently tracks total bytes read and aborts (without calling `end()`) on a mismatch — but this is a latent defect in the reusable `OtaUpdate` class itself: any future or alternate caller relying on the class's own documented behavior, rather than re-implementing the same completeness check itself, would silently accept a truncated image.

**H4. `modbus_pdu_parse_write_response()` doesn't validate response length, only the function code.**
Real Modbus write-acknowledgment responses (write single coil / write single register / write multiple registers) are always 5 bytes (function code + 2-byte address + 2-byte value-or-quantity). This function only requires `pdu_len >= 2`. A truncated or corrupted 2–3 byte response whose first byte happens to match the expected function code is accepted as a successful write confirmation, with nothing about the echoed address/value actually checked. This code path has zero test coverage.

### Medium

**M1. W5500 Ethernet has no hardware reset GPIO wired, apparently contradicting the subsystem's own documented design intent.**
`ethernet_link.cpp` passes `-1` (unused) for the W5500's reset pin. A subsystem design doc states, as an explicit, deliberate, dated design decision, that the reset line was wired to a dedicated MCU GPIO specifically so firmware could recover a hung W5500 without a full power cycle. But `docs/architecture.md`'s actual pin-assignment table only lists 5 Ethernet signals (CS/MOSI/SCLK/MISO/INT) with no reset pin, and `pin_map.h` has no corresponding `#define`. There's also a total/count inconsistency: `architecture.md`'s own changelog claims "28 cross-subsystem signals" assigned, but the actual table sums to 27. This is either a hardware pin-planning gap that needs a real GPIO assigned, or a stale design-intent doc that needs to be walked back — worth resolving one way or the other before board fabrication.

**M2. `pca9535`'s internal output-state shadow isn't synced from the chip's real power-on-reset state before the first `writePin()` call.**
The driver's `output_shadow_` defaults to `0x0000`, but the PCA9535's real power-on-reset Output Port register value is `0xFF`/`0xFF` (all bits high). `begin()` only does an I2C address ACK probe — it never reads the chip's actual current output register state. `writePin()` does a read-modify-write against `output_shadow_`, so calling it before any `writeOutputs()` call would silently drive all other output-configured pins low, clobbering whatever the chip's actual state was. Currently only avoided by accident — the one real call site (`status_leds`) happens to call `writeOutputs(0xFFFF)` before ever calling `writePin()` — not something the driver itself guarantees, and the driver's own header explicitly advertises itself as a generic, reusable component.

**M3. No anti-rollback/downgrade protection on OTA updates.**
The OTA trigger payload's `version` field is explicitly documented and implemented as informational-only, never used in any security or control-flow decision. A validly-signed OLD firmware image (a legitimate hash+signature pair from a previous release, including one with a since-fixed vulnerability) can be replayed indefinitely — the hash/signature checks only prove "an image that was once signed," not "the newest image that was signed." Worth at minimum recording as an accepted risk, or adding a monotonic minimum-version check.

**M4. `modbus_master` has some shared state outside its mutex protection, and no defensive null-check on the mutex in the read path.**
The mutex is correctly applied to the shared readings cache on both the write path (inside the polling task) and the read path (`getReading()`). However, `targets_[]`, `target_count_`, and `last_polled_millis_[]` are read by the polling task with no locking at all, while being written by `addTarget()` (presumably called from the main/setup context) — if a target is ever added after the task has started (not the documented intended usage today, but nothing prevents it), this is an unprotected data race. Separately, `getReading()` calls `xSemaphoreTake()` on the mutex handle with no null-check; if it's ever called before `begin()` has run, the mutex handle is still null, which is undefined behavior on real FreeRTOS.

**M5. Boot-time OTA rollback confirmation happens unconditionally and very early, undercutting its practical value.**
`confirmValid()` (cancelling the bootloader's pending-rollback state) is called unconditionally near the very start of `setup()`, based only on "the firmware got this far without crashing." The underlying rollback *primitive* is correctly implemented — but this means the safety net only protects against a crash/hang before `setup()` even runs, not against a new firmware image that boots cleanly but then fails to actually connect to WiFi/MQTT, or has some other functional regression — arguably the more common real-world class of bad OTA update.

**M6. `mqtt_client_wrapper`'s offline outbox can deliver messages out of chronological order under a specific failure pattern.**
The outbox always places a newly-queued message into the first free slot, and flushes slots in order, stopping at the first failure to preserve order. If an interior (non-tail) queued message repeatedly fails to publish (e.g., a payload that's permanently too large) while slots ahead of it have already drained and been freed, subsequently-queued newer messages backfill those freed earlier slots and get sent before the still-stuck older message on the next flush — breaking the documented FIFO/ordering guarantee.

**M7. `Serial.begin()` / UART0-vs-USB-CDC console routing is unconfirmed, with a potential pin conflict.**
`pin_map.h` puts RELAY3/RELAY4 on GPIO43/44 (the module's default UART0 TX/RX), justified by the comment "UART0 unused" (i.e., the intent is that the Arduino `Serial` console goes over the ESP32-S3's native USB-CDC instead of physical UART0). But `platformio.ini` sets no explicit `ARDUINO_USB_CDC_ON_BOOT`/`ARDUINO_USB_MODE` build flag, so which one `Serial` actually resolves to depends on the board manifest's own default — unconfirmed. If it resolves to physical UART0, it would conflict with RELAY3/4.

### Low

**L1. `power_safety` defaults to "no extra startup delay" specifically when NVS fails to open**, which is arguably the wrong direction for a module whose purpose is protecting against marginal/brownout power conditions — a failing NVS open is itself a plausible symptom of exactly that condition.

**L2. `event_log` writes tag/message fields unescaped into a comma-separated log line.** A value containing a comma or embedded newline would inject extra fields or fabricate what looks like a separate log entry. Not currently reachable (all current call sites pass fixed string literals), but a latent risk if any future call site logs attacker-influenced content verbatim.

**L3. `mqtt_client_wrapper`'s `publish()`/`enqueue()` don't null-check `topic`/`payload`** (unlike `subscribe()`, which does check). No current call site passes null, so this is a defensive-coding gap rather than a live bug today.

**L4. `ota_trigger`'s numeric-field parser has an unstated dependency on `unsigned long` being 32 bits.** Safe on the real ESP32-S3 target (where `unsigned long` is 32-bit, so out-of-range values saturate and get caught by the subsequent bounds check), but on a 64-bit host build of the same parser, a sufficiently large crafted value could avoid saturation and then get silently truncated by the later narrowing cast to `uint32_t`, potentially slipping past the intended size bound. The existing test for "oversized size field" doesn't test anywhere near the actual 32-bit boundary.

**L5. `ota_session`'s download loop doesn't independently cap a single read to the remaining declared image size** (it's bounded by a fixed local buffer, just not by "how many bytes are left to reach the declared total"). Not currently exploitable — the underlying `Update` write function and a final total-bytes check both catch the mismatch — but a compliant server sending exactly the declared size immediately followed by more data (coalesced into one TCP read) would trigger an avoidable abort rather than a clean stop.

**L6. `image_verify`'s SHA-256 streaming calls (`mbedtls_sha256_starts`/`mbedtls_sha256_update`) don't check their return codes.** These can theoretically report a hardware-acceleration failure. In practice this just produces a wrong hash, which is then caught by the (checked) hash-comparison step afterward — fails safe, not a security issue, just a robustness gap.

**L7. Minor buffer-size mismatch between `ota_trigger`'s maximum accepted signature length and `ota_session`'s actual decode buffer size** — a syntactically-valid-per-the-parser signature near the parser's own upper bound would be rejected later by the decode step instead of by the parser itself. Not reachable by a real ECDSA-P256 signature (which is far shorter than either limit), just an inconsistency between the two modules' stated bounds.

**L8. `StatusLed::HEARTBEAT` is defined in the status-LED enum but never actually driven anywhere in `main.cpp`.**

**L9. `McpAnalogOutput` (the DAC driver) is initialized in `setup()` but has no consumer anywhere** — nothing in `loop()`, and no rule-engine action type targets it. Currently dead code (not itself harmful, since the DAC's own power-on state is a safe 0V, but worth knowing).

### Test coverage gaps

**T1.** `modbus_pdu`: several of the read-request builder functions, and both response-parsing functions (bit-response and write-response), have zero test coverage.
**T2.** `modbus_rtu`/`modbus_tcp`/`io_state`: a handful of edge cases (too-short frame, buffer-too-small, out-of-range channel index, null-pointer args) are implemented correctly but have no test exercising them.
**T3.** `ota_update.cpp`'s actual logic (rollback state query/confirm/reject, `begin`/`write`/`end`) has zero test coverage — only a pure, hardware-independent progress-percentage helper function is tested.
**T4.** `power_safety::begin()`, including its NVS-failure fallback path (see L1), has zero test coverage.
**T5.** `url_host` has no test covering the `@`-userinfo URL shape that exposes H1.
**T6.** No test exercises `ota_trigger`'s numeric-field parser anywhere near the actual 32-bit boundary described in L4.

### Documentation debt (docs vs. actual repo state — not code bugs)

**D1.** `docs/roadmap.md` and `CLAUDE.md` both still describe the `platformio.ini` flash-size/PSRAM board override as "still open" / not yet done. It's already present in the file (`board_build.flash_size = 16MB`, `board_build.arduino.memory_type = qio_opi`) — the docs were never updated after the fix landed.
**D2.** Both docs have sections (from an earlier project phase) still describing an older, since-superseded version of `lte_modem` that included AT-command/UART logic and a separate `lte_at_parse` parsing module with its own tests — that code was later deleted and `lte_modem` trimmed to power/reset-only, which a *later* section of the same two docs correctly describes. The two sections of each doc now contradict each other.
**D3.** Several "N checks, all passing" test-count claims in the docs don't match the actual number of test cases in the corresponding `firmware/test/` file (specific modules affected: `ota_trigger`, `url_host`, `net_allowlist`, `rule_engine` — each off by a few from what's actually in the file).
**D4.** Three specific verification claims in the docs (for `image_verify`, for the LTE/PPP module pairing, and for the network-priority fallback logic in `main.cpp`) describe passing behavior tests with a specific check count, but there is no corresponding test file anywhere in the repository for any of the three — meaning those verification passes, even if they genuinely happened, are not currently reproducible from the repo alone.
**D5.** `docs/roadmap.md` still lists a digital-input strapping-pin question (GPIO3/GPIO46 usage) as an open decision in one section, while `CLAUDE.md` records it as already resolved (reviewed and accepted) in another — the two docs disagree with each other on the current status of the same decision.

---

*End of review request. Reminder: report findings only — do not modify any file in this repository.*
