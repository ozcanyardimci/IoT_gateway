# Build log

## 2026-08-28 — Project initialized
- Repository structure created (hardware/, firmware/, docs/).
- ESP32-S3 fixed pin/peripheral constraints confirmed (see docs/architecture.md).
- PlatformIO project scaffolded in firmware/, configured to use the pioarduino
  platform (github.com/pioarduino/platform-espressif32) instead of the stock
  PlatformIO Espressif32 platform, to track current Arduino-ESP32 core releases.
- Toolchain sanity check (blink sketch) added at firmware/src/main.cpp — to be
  built and flashed before any real subsystem firmware work starts.

## 2026-09-03 — Power subsystem complete
- Reverse-polarity protection, surge/EMI protection, and four-rail regulation (3.3V-LOGIC,
  3.3V-LTE, 5V-RELAY, 3.3V-ANALOG-ISO) designed and simulated (ngspice inrush, margin
  verification against every part's worst-case rating).
- Schematic captured in `hardware/kicad/ioboard/` (J1 -> F1 -> Q5/U1 -> U2 -> four DC-DC
  modules; the reverse-polarity MOSFET was later renamed from Q1 to Q5 during the analog-io
  subsystem's ERC pass, to resolve a duplicate-reference collision — see `power.md`'s
  revision history). Full BOM sourced to real, in-stock part numbers.
- See `docs/subsystems/power.md` for the full writeup; items needing real hardware are
  tracked there as commissioning tests for Rev-A.

## 2026-09-04 — Core compute subsystem complete
- ESP32-S3-WROOM-1U-N16R8 bring-up: power/decoupling, EN reset circuit, GPIO0 boot circuit,
  USB-C interface (data-only, CC termination, no VBUS power path) wired pin-by-pin against
  Espressif's own hardware design guidelines and esptool's boot-mode documentation.
- Schematic captured in `hardware/kicad/lteboard/lteboard/core-compute.kicad_sch`. Caught
  and fixed a real gap here: the sheet had only been auto-saved, not explicitly saved, so
  the first commit of this file was accidentally empty — corrected once the file was
  properly saved and the real 3444-line schematic was verified against the documented
  values before committing.
- See `docs/subsystems/core-compute.md` for the full writeup.

## 2026-09-05 — Digital inputs subsystem complete
- 8 opto-isolated digital inputs across two LTV-247 ICs: field input -> current-limiting
  resistor (2.4k) -> LED -> reverse-protection diode (1N4148) -> GND_FIELD_DI, output side
  -> pull-up (10k) to 3V3_LOGIC + filter cap (100nF) to GND_LOGIC. Field connector (Phoenix
  Contact MC 1,5/9-ST-3,5) terminates all 8 channels plus the shared common return.
- Schematic captured in `hardware/kicad/ioboard/ioboard/digital_inputs.kicad_sch`. Every
  channel's pin numbers, label spelling, and diode orientation verified against the actual
  placed KiCad symbols (not assumed from a datasheet pinout diagram, which couldn't be
  reliably sourced for this part) before moving to the next channel.
- Established a clearer project-wide label-scoping convention: local labels for anything
  that never leaves one sheet, hierarchical labels plus an explicit wire on the parent sheet
  for anything shared between a specific set of subsystems, global labels reserved for
  GND_LOGIC only. Retroactively wired 3V3_LOGIC between power and digital_inputs through
  ioboard.kicad_sch's root sheet using this convention.
- See `docs/subsystems/digital-inputs.md` for the full writeup.

## 2026-09-06 — Relay outputs subsystem complete
- 4 relay channels: GPIO -> 510Ω base resistor + 10kΩ pull-down -> MMBT3904 NPN driver ->
  Panasonic ALDP105 relay coil (`5V_RELAY` rail), with a 1N4148 flyback diode across each
  coil. Local 10uF/100nF bulk+bypass caps at the `5V_RELAY` entry point, for EMI/noise
  containment rather than current capability.
- Field-side contacts terminate on an 8-position Phoenix Contact MC 1,5/8-ST-3,5 connector,
  giving every channel a fully independent COM+NO pair (no shared common return, unlike
  digital-inputs).
- Schematic captured in `hardware/kicad/ioboard/ioboard/relay_outputs.kicad_sch`. One real
  wiring bug caught and fixed before commit: the flyback diode was initially wired with
  reversed polarity, which would have put a forward-biased diode across the coil for the
  entire on-time of each transistor.
- Known limitation, documented rather than silently absorbed: the field connector is rated
  160V while the relay's own contacts are rated to 277VAC/30VDC — the connector, not the
  relay, is the practical ceiling on what this board can switch.
- See `docs/subsystems/relay-outputs.md` for the full writeup.

## 2026-09-07 — Status indication subsystem complete
- I2C GPIO expander (NXP PCA9535PW, already budgeted in `power.md`) driving 6 status LEDs
  (PWR-OK, HEARTBEAT, LTE, WIFI, ETH, FAULT), active-low sink drive. Address strapped to 0x20
  (A0/A1/A2 to `GND_LOGIC`); 4.7kΩ pull-ups on SDA/SCL; 10kΩ pull-up on the unused INT pin;
  10 spare GPIOs marked with No-Connect flags. LED colors deliberately limited to the
  red/green/yellow/orange family — blue/white were ruled out because their higher forward
  voltage leaves too little headroom on the 3.3V rail for reliable current control.
- New second hierarchical sheet on LTEBOARD (`status_indication.kicad_sch`), wired directly
  to the ESP32-S3 (GPIO8/GPIO9 for I2C, exposed as new sheet pins on `core-compute.kicad_sch`)
  — no board-to-board header involved, since both sheets live on the same physical board.
- Caught and fixed two real bugs before commit, both via full programmatic connectivity
  verification (every pin and net reconstructed from the actual file content, not just
  visual inspection): all 6 LEDs were initially wired with reversed polarity (would never
  have lit, since the cathode side had no path to a potential low enough for forward bias);
  and `core-compute.kicad_sch`'s existing `GND_LOGIC` labels were local instead of global —
  same class of bug found and fixed in `power.kicad_sch` during analog-io's ERC pass, which
  would have left this new sheet's ground electrically isolated from the MCU's.
- ERC deferred to a single project-wide pass once every subsystem is built, a project-wide
  decision replacing the per-subsystem ERC pass used for analog-io.
- See `docs/subsystems/status-indication.md` for the full writeup.

## 2026-09-07 — Analog I/O subsystem complete
- 2 analog inputs (0-10V / 4-20mA stuffing option) + 1 analog output (0-10V), isolated from
  GND_LOGIC: ADS1115 (ADC) + MCP4725 (DAC) share one I2C bus on the isolated side, crossing
  to GND_LOGIC through an ISO1540 bidirectional isolator. Output stage is a real DAC + LM2904
  gain stage (gain 3.004), not PWM+filter — the ESP32-S3 has no internal DAC. Surfaced a real
  gap in the existing rail budget: the 0-10V output needs a new isolated ~15V rail, sourced
  from 5V-RELAY. That gap was closed separately on `main` (see the 2026-09-08 entry below)
  before this branch was merged back in.
- Schematic captured in `hardware/kicad/ioboard/ioboard/analog_io.kicad_sch`. One real wiring
  bug (output-stage feedback network wired to break the loop) and one naming ambiguity
  (channel-1/channel-2 resistor designators swapped) caught and fixed during capture.
- Full KiCad ERC (Electrical Rules Check) run to a fully-explained clean state: 15 findings
  reviewed and excluded with a written, per-violation comment (cross-sheet power-driver false
  positives, documented spare/deferred headroom, deferred GPIO assignment, the deferred 15V
  rail) — nothing silently suppressed. This pass also surfaced and fixed two pre-existing
  bugs outside this sheet: a duplicate `Q1` reference collision in `power.kicad_sch` (renamed
  to `Q5`) and an inconsistent local/global `GND_LOGIC` labeling on the same sheet (both
  recorded in `power.md`'s revision history, not just here).
- See `docs/subsystems/analog-io.md` for the full writeup.

## 2026-09-08 — Power: isolated 15V rail for analog output stage
- Solved the 15V problem flagged during analog-io's design: added U7 (Recom RK-0515S,
  isolated DC-DC, 4.5-5.5V in / 15V out / 66mA, 3kVDC isolation) to `power.kicad_sch`,
  powered from `5V-RELAY`. Verified wiring programmatically (+VIN->5V_RELAY,
  -VIN->GND_LOGIC, +VOUT->15V_ANALOG_ISO, -VOUT->GND_ANALOG_ISO), confirming -VIN and
  -VOUT resolve to two distinct nets — isolation preserved.
- Completed the root-sheet routing in `ioboard.kicad_sch`: added a `15V_ANALOG_ISO` output
  pin to the power sheet symbol and wired it through to `analog_io`'s existing
  `15V_ANALOG_ISO` input pin (on U13's V+, already anticipated in that branch's design).
- Corrected the LM2904 load-budget entry in `power.md`: the old single row put both op-amp
  packages on 3.3V-LOGIC with an imprecise per-amp current figure. Split into U12
  (input-buffer stage, 3.3V-ANALOG-ISO) and U13 (output gain stage, new 15V-ANALOG-ISO
  rail), both at the TI-datasheet-verified 0.7-1.2 mA dual-package draw.
- See `docs/subsystems/power.md`'s revision history for the full writeup.


## 2026-09-10 — RS485 and RS232 subsystems complete
- **RS485 (isolated):** Mornsun TD321S485H-A transceiver module, copying Mornsun's own
  Fig. 2 harsh-environment reference circuit exactly (GDT + series R + TVS + common-mode
  choke + external 4.7kΩ bias). Phoenix Contact MC 1,5/3-ST-3,5 connector. Schematic
  captured in `hardware/kicad/ioboard/ioboard/rs485.kicad_sch`.
- **RS232:** TI MAX3232EIPWR front end (switched from an initially-misidentified
  "MAX3232EI" part number, not a real orderable SKU), no added protection network (the
  receiver's own ±25V-safe-unpowered rating plus reference-design evidence supported
  skipping one), reusing RS485's connector family. Schematic captured in
  `hardware/kicad/ioboard/ioboard/rs232.kicad_sch`.
- ERC policy formalized for both and going forward: deferred to a single project-wide pass
  once every subsystem is built, rather than per-subsystem before each merge — matching
  status-indication's approach, superseding the earlier per-subsystem policy.
- See `docs/subsystems/rs485.md` and `docs/subsystems/rs232.md` for full writeups.

## 2026-09-13 — Ethernet and WiFi subsystems complete
- **Ethernet:** WIZnet W5500 SPI controller, Würth 7499010441 magjack — exact part number
  and internally-terminated construction confirmed directly from its own datasheet (no
  external Bob-Smith network needed). Schematic captured in
  `hardware/kicad/lteboard/lteboard/ethernet.kicad_sch`. One label typo found (C7,
  `1nF/2kW` should read `1nF/2kV`) — fixed 2026-09-23, see below.
- **WiFi:** small scope — the ESP32-S3-WROOM-1U's radio is already fixed at the module;
  this subsystem was just the external antenna path (SMA jack, coax pigtail, gain-limited
  2dBi antenna to stay under the module's 2.33dBi certified-antenna ceiling), added
  directly to `core-compute.kicad_sch`.
- See `docs/subsystems/ethernet.md` and `docs/subsystems/wifi.md` for full writeups.

## 2026-09-19 — LTE subsystem complete; both boards merged into one KiCad project
- **LTE:** Quectel EG915U-EU, the largest/most complex subsystem — own rail (`VBAT_LTE`,
  retapped from ~3.3V to ~3.82V for real margin against the TX transient, see `power.md`),
  own UART through a TXB0102 level shifter, SIM interface, PWRKEY/RESET power-control
  circuit, own antenna path. Schematic captured in
  `hardware/kicad/lteboard/lteboard/lte.kicad_sch`.
- All 11 subsystems now schematic-complete. `ioboard/` and `lteboard/` merged into one
  active project, `hardware/kicad/ioboard+lteboard/`, so roadmap steps 6 (fine-grained pin
  assignment) and 7 (board-to-board wiring) could be done with both boards visible at
  once. `ioboard/`/`lteboard/` frozen as historical record from this date. Full detail:
  `docs/board-merge.md`.
- See `docs/subsystems/lte.md` for the full writeup.

## 2026-09-22/23 — Roadmap steps 6 and 7 closed; project-wide ERC clean; firmware starting
- **Step 6 (fine-grained pin assignment):** all 28 cross-subsystem signals given real
  GPIOs on `core-compute.kicad_sch` — I2C, Ethernet SPI, LTE UART+control, RS232/RS485
  UARTs, 8 digital inputs, 4 relay outputs. Full table in `docs/architecture.md`. A
  pre-existing defect was caught and fixed in the same pass: USB-C D+/D- were wired to the
  wrong GPIOs (ordinary pins instead of the module's actual native-USB pins), meaning USB
  data would never have worked as originally built. Five DI signals landed on GPIO3/39/
  42/46/47 — checked against Espressif's official ESP32-S3 datasheet for strapping/JTAG
  concerns; accepted as safe (two are real strapping pins but safe for this use, three
  were incorrectly flagged as risky in earlier project notes and are actually plain
  GPIOs). Analog output's DAC-vs-PWM question, open since early planning, was confirmed
  already decided in practice: MCP4725 I2C DAC, not PWM+filter.
- **Step 7 (board-to-board wiring):** two connector pairs added (`J_PWR` 2x5, `J_SIG`
  2x12, Samtec TSW/SSW 2.54mm) carrying `3V3_LOGIC`/`VBAT_LTE`/`GND_LOGIC`/`EARTH` and 18
  of 24 signal pins (6 spare, reserved). Full pinout in `docs/architecture.md`.
- **Final project-wide ERC pass:** found and fixed real defects across several sheets —
  two SnapEDA-imported power-module symbols (PS1/RK-0515S, U13/R1SX-3.33.3-R) had their
  return-side output pin mistyped, reading as a driven-output conflict; five isolated
  power/ground nets needed `PWR_FLAG`s their symbol-level connectivity couldn't establish
  across sheets; the LTE modem's VDD_EXT/USIM1_VDD pins were mistyped power-input instead
  of power-output; three genuinely-floating Ethernet PMODE pins got pull-up resistors;
  several genuinely-unused pins (ADS1115, core-compute) got No-Connect flags instead of
  being left ambiguous. ERC confirmed clean 2026-09-23.
- `firmware/platformio.ini` corrected to declare the real ESP32-S3-WROOM-1U-N16R8 module
  (16MB flash, 8MB octal PSRAM) instead of a generic N8 devkit profile.
- Full reasoning trail for all of the above: `CLAUDE.md`'s open items and each affected
  subsystem doc's revision history.
- **Firmware development starts from here.**

## 2026-09-23/24 — Firmware F1-F4 written, all modules compile/gcc-verified

- Full firmware implementation written against the real, merged schematic's pin
  assignments: protocol/logic layer (Modbus RTU/TCP, MQTT payloads, DI/AI/relay
  register mapping), all 10 hardware drivers, and the industrial-grade layer
  (commissioning portal, MQTT resilience + TLS, local rule engine, OTA with
  rollback, NTP, event logging, WireGuard VPN, an application-layer outbound
  allowlist, LTE registration resilience, brownout safe-state handling).
- System integration (F4): FreeRTOS task scheduler split (dedicated Modbus RTU
  polling task + cooperative `main.cpp` loop for everything else), task watchdog,
  WireGuard config storage/wiring, a from-scratch OTA update-trigger flow with
  application-level ECDSA signature verification (mbedtls, after two "obvious"
  ESP-IDF/arduino-esp32 signing mechanisms were researched and ruled out as
  actually broken or inapplicable under `framework = arduino`), and LTE backhaul
  via arduino-esp32's built-in PPP library as a real third connectivity tier
  (`Ethernet -> LTE -> WiFi`, Ozcan's explicit priority call).
- Verification at this stage was write + compile/gcc-verify only — real hardware
  didn't exist yet, and Ozcan's own call (2026-09-24) was to defer the actual
  `pio run` build/link pass until the whole of F4 was finished, then do it once,
  together, against the complete integrated firmware rather than piecemeal.
- Full module-by-module detail: `docs/roadmap.md`'s "Firmware roadmap" section
  (F1-F4).

## 2026-09-25 — First real `pio run`/`pio test` build verification — F4 closed

- Ran the real build on Ozcan's machine (WSL, real `pio` binary) for the first
  time against the complete F1-F4 firmware above. Took 4 iterations to reach a
  clean build/link — a duplicate `libsodium` install colliding at the SCons
  level, a missing project-wide include path for `pin_map.h`, three stray
  `void`-to-`bool` checks in `main.cpp`'s boot sequence, and a nested-vs-top-level
  `MqttOutbox` class mismatch left over from an earlier library split — none of
  them catchable by gcc/g++ stub-compiling alone, all specific to the real
  ESP32-S3 build path.
- `pio run -e esp32-s3-devkitc-1` now succeeds end to end: links `firmware.elf`,
  builds `firmware.bin`/`firmware.factory.bin` (RAM 21.8%, Flash 32.7% against the
  build's reported budget).
- `pio test -e native` ran for the first time against the real PlatformIO/Unity
  test runner — **132 of 132 test cases passed** across all 15 native-testable
  modules.
- This is the first real, non-gcc-stub verification of the whole codebase.
  Hardware still isn't required for any of it — Rev-A prototype PCB (roadmap step
  8) remains the next physical milestone, and the only thing gating F5
  (functional hardware bring-up).
- Full detail: `docs/roadmap.md`'s F4.1 section.
