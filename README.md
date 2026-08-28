# IoT_gateway

A 4G/LTE industrial IoT gateway, designed and built from the ground up as a hands-on
learning project spanning electronics, PCB design (KiCad), and embedded firmware
(ESP32-S3, PlatformIO/Arduino).

## What it does

- LTE Cat-1 cellular connectivity (with 2G fallback), WiFi, and 10/100 Ethernet
- 8x optically isolated digital inputs, 4x relay outputs
- Analog I/O (0-10V / 4-20mA input, 0-10V output)
- Isolated RS485 and RS232 serial interfaces
- 10-30 VDC field power input, DIN-rail enclosure target
- Firmware targets: web UI, Modbus TCP/RTU, MQTT, HTTP, and eventually a WireGuard VPN tunnel

## Architecture

The design is split across two physically separate PCBs connected by a board-to-board
header:

- **Communications board** — ESP32-S3 (main MCU + WiFi), LTE modem, Ethernet controller,
  I2C GPIO expander for status indication. Kept isolated from switching noise.
- **I/O board** — power regulation, relay outputs, RS485/RS232 transceivers, analog
  input/output signal conditioning. Kept isolated from RF/clock-sensitive circuitry.

Separating the two is a deliberate noise-isolation decision, not just a physical split for
its own sake: RF and clock-sensitive circuitry (LTE/WiFi/Ethernet) is kept away from
switching-noisy circuitry (relay coils, analog front-end).

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
  build-log.md          Incremental build/bring-up log
```

## Build methodology

This project is built subsystem-first: each subsystem (power, core compute, WiFi, LTE,
Ethernet, digital I/O, relay outputs, analog I/O, RS485, RS232, status indication) is
validated in isolation (simulation-first: KiCad/ngspice for power and analog circuits,
Wokwi for ESP32-S3 firmware/driver logic) before being consolidated into the final
two-board design. A small Rev-A prototype PCB is planned as the first real hardware
checkpoint, ahead of committing to the full two-board fabrication run.

See `docs/architecture.md` and `docs/build-log.md` for details as the project progresses.

## Status

Early stage — hardware/firmware toolchain set up, component selection and protection
design in progress. Not yet built or tested on real hardware.

## License

TBD.
