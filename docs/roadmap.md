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

## 4. Isolated subsystem prototyping — IN PROGRESS

Simulation only (KiCad/ngspice for power and analog circuits). Wokwi isn't used going
forward — most real subsystems here (LTE, Ethernet, RS485, RS232) can't be simulated in it;
that firmware is written against datasheets and validated at Rev-A instead.

Each subsystem gets its own build plan under `docs/subsystems/` before work starts on it.
Order:

1. Power — `docs/subsystems/power.md` — **DONE**
2. Core compute (ESP32-S3 bring-up)
3. Digital inputs (8x, opto-isolated)
4. Relay outputs (4x)
5. Status indication (I2C GPIO expander + LEDs)
6. Analog I/O (input + output)
7. RS485 (isolated)
8. RS232
9. Ethernet (W5500)
10. WiFi
11. LTE (Quectel EG915U-EU) — most complex, done last

## 5. Per-subsystem acceptance criteria — DONE FOR POWER, PER-SUBSYSTEM GOING FORWARD

Each subsystem's own plan now defines its pass/fail acceptance criteria as part of that
subsystem's build (see `docs/subsystems/power.md`, step 11).

## 6. Fine-grained resource planning — NOT STARTED

Exact pin/bus assignment across the full design. Also where `platformio.ini`'s board
variant gets corrected to match the real N16R8 module (currently a generic N8 profile), and
where the analog-output DAC-vs-PWM decision gets finalized.

## 7. Incremental consolidation — NOT STARTED

Firmware: git branches per subsystem, merged incrementally. Hardware: KiCad hierarchical
sub-schematics per subsystem. Includes a dedicated board-to-board interconnect test
milestone.

## 8. Rev-A prototype PCB spin — NOT STARTED

First point real physical hardware is needed.

## 9. Integration schematic + PCB layout — NOT STARTED

## 10. Fabrication, bring-up, enclosure, iterate — NOT STARTED
