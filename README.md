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
- Firmware targets: web UI, Modbus TCP/RTU, MQTT, HTTP, and eventually a WireGuard VPN tunnel

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

Seven subsystems complete on paper — power, core compute (ESP32-S3 bring-up), digital
inputs (8x, opto-isolated), relay outputs (4x), status indication (I2C GPIO expander + 6
LEDs), analog I/O (2 in / 1 out, isolated), and RS485 (isolated) — protection, regulation,
part selection, and schematic capture done for each, verified pin-by-pin against
manufacturer documentation. RS232 is next. Not yet built or tested on real hardware.

## License

TBD.
