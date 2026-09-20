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

## 6. Fine-grained resource planning — NOT STARTED

Exact pin/bus assignment across the full design. Also where `platformio.ini`'s board
variant gets corrected to match the real N16R8 module (currently a generic N8 profile), and
where the analog-output DAC-vs-PWM decision gets finalized. Happens in `ioboard+lteboard`
now (see step 4 and `docs/board-merge.md`), since the pins being assigned on
`core-compute.kicad_sch` serve subsystems on both physical boards.

## 7. Incremental consolidation — IN PROGRESS

Firmware: git branches per subsystem, merged incrementally. Hardware: KiCad hierarchical
sub-schematics per subsystem — done per-board already, and both boards' hierarchies were
brought into one schematic project 2026-09-19 (`docs/board-merge.md`). What's still
outstanding is the actual board-to-board interconnect test milestone: `3V3_LOGIC` and
`VBAT_LTE` still need to be tied across the two board-side islands the merge revealed, and
that wiring needs a real decision on how the board-to-board connector itself is
represented in the schematic before it's just a plain wire.

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
