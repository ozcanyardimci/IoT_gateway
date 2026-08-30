# Build roadmap

10-step plan we're following, in order. Status is updated as we go.

## 1. Repo + tooling setup — DONE
GitHub repo created and pushed (`IoT_gateway`). VS Code + PlatformIO + pioarduino + KiCad +
Wokwi extension installed. PlatformIO build confirmed working end to end (pioarduino resolved
and installed, build succeeds).

## 2. Fixed chip-level constraints check — DONE
Locked in before any design work, because these can't change later without a different MCU
module:
- Module: ESP32-S3-WROOM-1U-**N16R8** = 16MB Quad SPI flash + 8MB **octal-mode** PSRAM.
- GPIO26-32 permanently reserved (flash).
- GPIO33-37 permanently reserved (octal-mode PSRAM).
- GPIO0 / 45 / 46 / 3 are strapping pins (avoid for general I/O).
- GPIO39 / 42 / 47 / 21 have nonstandard reset behavior.
- Exactly 3 hardware UART controllers available.
- No built-in DAC peripheral (removed vs. original ESP32/S2) — analog output needs
  PWM+filter or an external DAC chip, decided at step 6.

Full detail in `architecture.md` under "Fixed MCU constraints."

## 3. One-page block diagram — DONE (draft)
Mermaid diagram + bus assignment table + board-to-board header signal count, in
`architecture.md`.

## 4. Isolated subsystem prototyping — NOT STARTED
Simulation only (KiCad ngspice for power/analog, Wokwi for firmware/driver logic). No
breadboard, no real hardware at this stage.

## 5. Per-subsystem acceptance criteria — NOT STARTED
Define what "this subsystem works" means for each block before building it.

## 6. Fine-grained resource planning — NOT STARTED
Exact pin/bus assignment across the full design. Also where the `platformio.ini` board
variant gets corrected to match the real N16R8 module (currently defaults to a generic N8
profile), and where the analog-output DAC-vs-PWM decision gets finalized.

## 7. Incremental consolidation — NOT STARTED
Firmware: git branches per subsystem, merged incrementally. Hardware: KiCad hierarchical
sub-schematics per subsystem. Includes a dedicated board-to-board interconnect test
milestone (LTEBOARD ↔ IOBOARD header has ~20+ signal lines, not just power).

## 8. Rev-A prototype PCB spin — NOT STARTED
First point real physical hardware is needed.

## 9. Integration schematic + PCB layout — NOT STARTED

## 10. Fabrication, bring-up, enclosure, iterate — NOT STARTED
