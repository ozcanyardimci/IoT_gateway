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
- 4 relay channels: GPIO -> base resistor (510) + pull-down (10k) -> NPN driver transistor
  (MMBT3904) -> ALDP105 relay coil, with a flyback diode (1N4148) across each coil.
  Field-side contacts terminate on an 8-position Phoenix Contact connector giving every
  channel a fully independent COM+NO pair (no shared return, unlike the analog subsystem).
- Schematic captured in `hardware/kicad/ioboard/ioboard/relay_outputs.kicad_sch`.
- See `docs/subsystems/relay-outputs.md` for the full writeup.

## 2026-09-07 — Analog I/O subsystem complete
- 2 analog inputs (0-10V / 4-20mA stuffing option) + 1 analog output (0-10V), isolated from
  GND_LOGIC: ADS1115 (ADC) + MCP4725 (DAC) share one I2C bus on the isolated side, crossing
  to GND_LOGIC through an ISO1540 bidirectional isolator. Output stage is a real DAC + LM2904
  gain stage (gain 3.004), not PWM+filter — the ESP32-S3 has no internal DAC. Surfaced a real
  gap in the existing rail budget: the 0-10V output needs a new isolated ~15V rail, sourced
  from 5V-RELAY; that rail's addition to `power.md` is deferred to `main`, after this
  subsystem branch merges.
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
