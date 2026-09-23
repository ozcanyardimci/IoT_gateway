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
