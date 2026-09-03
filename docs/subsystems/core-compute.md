# Core Compute Subsystem — Build Plan

**Status:** in progress. Requirements locked, schematic capture not started.

## Scope

ESP32-S3-WROOM-1U-N16R8 module bring-up on LTEBOARD: power/decoupling, reset (EN) and boot
(GPIO0) circuits, strapping pin handling, and the USB interface used for
programming/debug. Not in scope here: WiFi/LTE/Ethernet peripheral wiring (their own
subsystems), exact GPIO/bus pin assignment (roadmap step 6).

## Design approach

Mostly a digital/power-integrity design, not a circuit with a control loop — the plan
shape from the power subsystem is adapted, not copied: no protection-chain sequencing, no
inrush/surge simulation, no separate load-budget derivation (already done during power
subsystem work). Verification here is a datasheet-compliance checklist, not ngspice.

## Steps

1. **Requirements** — pull the module's real supply/reset/boot/USB requirements from
   Espressif's own documentation.
2. **Power & decoupling** — confirm the 3.3V-LOGIC rail (power subsystem) covers this
   module's requirements; place decoupling per Espressif's guidelines.
3. **Reset (EN) & boot (GPIO0) circuit** — RC reset delay, boot-mode pull-up, and buttons
   for manual reset/download-mode entry.
4. **Strapping pins** — confirm GPIO0/45/46/3 are left free of conflicting external pulls
   (cross-check against `architecture.md`'s fixed constraints).
5. **USB interface** — native USB (device) wiring for programming/debug, connector choice,
   series resistors.
6. **Antenna** — WROOM-1U variant uses an external antenna via U.FL connector, not a
   built-in PCB antenna — needs its own connector + RF trace note.
7. **Schematic capture (KiCad)** — new `lteboard` project, `core_compute.kicad_sch`.
8. **Verification checklist** — every value above checked against the datasheet, not
   simulated.
9. **Acceptance criteria.**
10. **Documentation & BOM.**
11. **Sign-off** — move to the next LTEBOARD subsystem.

---

## Step 1 results: requirements (2026-09-03)

Module: **ESP32-S3-WROOM-1U-N16R8** (fixed at project start — 16MB flash, 8MB octal PSRAM,
external-antenna variant).

| Item | Requirement | Source |
|---|---|---|
| Supply voltage | 3.0-3.6V (VDD3V3) | Module datasheet |
| Supply current | Already budgeted in power subsystem: ~95-100mA typical (WiFi RX), 355mA peak (WiFi TX burst) | `docs/subsystems/power.md` |
| EN (CHIP_PU) reset circuit | RC delay: R = 10k ohm, C = 1uF to GND | Espressif ESP32-S3 Hardware Design Guidelines, Schematic Checklist |
| GPIO0 (boot) circuit | GPIO0 has a 45k ohm internal pull-up — no external pull-up resistor is required. A boot button needs a *strong pull-down* (10k ohm to GND) when pressed, since 45k is too weak to reliably override with a switch alone | Espressif esptool documentation, Boot Mode Selection |
| Strapping pins | tH = 3ms after EN goes high before the chip reads strapping state — no external circuit may drive GPIO0/45/46/3 during that window | Espressif guidelines |
| USB D+/D- | Reserve unpopulated series resistors (22-33 ohm) and ground caps on the traces; no specific ESD diode called out in this document — adding one is still standard practice for a field-exposed USB port | Espressif guidelines |
| Decoupling | 0.1uF close to each digital power pin; 10uF on VDD3P3 (analog); >=10uF at the main power entrance to the module; all placed close to the pins | Espressif guidelines |

No open items on GPIO0 — resolved directly against Espressif's boot-mode documentation
rather than left as a guessed reference-design value.
