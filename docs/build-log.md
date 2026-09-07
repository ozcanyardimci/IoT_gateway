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
- Schematic captured in `hardware/kicad/ioboard/` (J1 -> F1 -> Q1/U1 -> U2 -> four DC-DC
  modules). Full BOM sourced to real, in-stock part numbers.
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
