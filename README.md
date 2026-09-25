# IoT_gateway

A 4G/LTE industrial IoT gateway, designed and built from the ground up as a hands-on
learning project spanning electronics, PCB design (KiCad), and embedded firmware
(ESP32-S3, PlatformIO/Arduino).

## What it does

- LTE Cat-1 cellular connectivity (with 2G fallback), WiFi, and 10/100 Ethernet
- 8x optically isolated digital inputs, 4x relay outputs
- Analog I/O (0-10V / 4-20mA input, 0-10V output)
- RS485 (galvanically isolated) and RS232 (not isolated — see `docs/subsystems/power.md`'s
  isolation table) serial interfaces
- 10-30 VDC field power input, DIN-rail enclosure target
- Firmware: web UI (commissioning portal), Modbus TCP/RTU master, MQTT (with TLS + resilience), OTA updates with rollback and signature verification, and a WireGuard VPN tunnel

## Architecture

Two physically separate PCBs connected by a board-to-board header, split for noise
isolation — RF/clock-sensitive circuitry stays off the same board as switching-noisy
circuitry:

- **Communications board** — ESP32-S3 (main MCU + WiFi), LTE modem, Ethernet controller,
  I2C GPIO expander for status indication.
- **I/O board** — power regulation, relay outputs, RS485/RS232 transceivers, analog
  input/output signal conditioning.

## Repository structure

```
hardware/
  kicad/              KiCad schematic + PCB projects (one per board)
  datasheets/          Component datasheets
  bom/                 Bill of materials
  reference-designs/   Vendor reference designs used during design
firmware/
  platformio.ini       PlatformIO project config (ESP32-S3, pioarduino platform)
  src/                 Firmware source
  include/, lib/       Headers / project-local libraries
docs/
  architecture.md       Block diagram and subsystem breakdown
  roadmap.md            Top-level 10-step plan and subsystem order/status
  build-log.md          Incremental build/bring-up log
  subsystems/           Per-subsystem build plans (requirements through sign-off)
```

## Build methodology

Built subsystem-first: each subsystem (power, core compute, WiFi, LTE, Ethernet, digital
I/O, relay outputs, analog I/O, RS485, RS232, status indication) is validated in isolation
before being consolidated into the final two-board design — simulation-first (KiCad/ngspice
for power and analog circuits), then a Rev-A prototype PCB as the first real hardware
checkpoint, ahead of committing to the full two-board fabrication run.

See `docs/architecture.md` and `docs/build-log.md` for details as the project progresses.

## Status

**Hardware:** all 11 subsystems complete on paper (power, core compute, digital inputs,
relay outputs, status indication, analog I/O, RS485, RS232, Ethernet, WiFi, LTE) —
protection, regulation, part selection, and schematic capture done for each, verified
pin-by-pin against manufacturer documentation. Both boards' schematics are merged into
one KiCad project (`hardware/kicad/ioboard+lteboard/`, see `docs/board-merge.md`),
fine-grained MCU pin assignment and the board-to-board connector are both wired, and a
full project-wide Electrical Rules Check passes clean (2026-09-23). Not yet built as
real hardware — Rev-A prototype PCB is the next physical milestone (`docs/roadmap.md`
step 8).

**Firmware:** the full protocol/driver/industrial/system-integration stack (Modbus,
MQTT+TLS, OTA with rollback and signature verification, WireGuard, rule engine, task
scheduler/watchdog, LTE-PPP backhaul, and more) is written and build-verified — a real
`pio run` links and flashes a complete image, and all 132 native unit tests pass
(2026-09-25). What's left needs the Rev-A hardware above: nothing in the firmware has
been exercised against real peripherals yet (`docs/roadmap.md`'s F5). See
`docs/build-log.md` and `docs/roadmap.md` for the full detail.

## License

TBD.
