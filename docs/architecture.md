# Architecture

## Status
Block diagram not yet drawn — this is the next planning step before any subsystem
design work starts. This file will hold the one-page block diagram (subsystems, buses,
board-1/board-2 split) once complete.

## Subsystem list
- Power (input protection, regulation)
- Core compute (ESP32-S3 bring-up)
- WiFi
- LTE (cellular modem)
- Ethernet
- Digital inputs (8x, opto-isolated)
- Relay outputs (4x)
- Analog I/O (input + output)
- RS485 (isolated)
- RS232
- Status indication (I2C GPIO expander + LEDs)

## Fixed MCU constraints (ESP32-S3)
- GPIO26-32: reserved, in-package SPI flash — never reuse
- GPIO33-37: reserved, in-package octal PSRAM — never reuse
- GPIO0/45/46/3: strapping pins — boot-mode sensitive, use with care
- GPIO39/42/47/21: nonstandard reset behavior
- 3 hardware UART controllers available (LTE + RS485 + RS232 fits if debug/console
  uses native USB instead of a 4th UART)
