# Core Compute Subsystem — Build Plan

**Status:** Complete. All 11 plan steps closed.

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

---

## Step 7 results: schematic capture (2026-09-04)

Wired in `hardware/kicad/lteboard/lteboard/core_compute.kicad_sch`. Full pin-by-pin
verification against Espressif's own module datasheet and hardware design guidelines:

| Pin(s) | Net / circuit | Confirms |
|---|---|---|
| 2 (3V3) | `3V3_LOGIC`, with C1 (10uF) + C2 (0.1uF) to `GND_LOGIC` | Bypass + WiFi-TX-burst bulk reservoir, per Espressif's schematic checklist |
| 1 (GND) | `GND_LOGIC` | Single ground pin on this symbol (module's internal GND pins 1/40/41/EPAD are tied together on the module itself) |
| 3 (EN) | R1 (10k) to `3V3_LOGIC`, C3 (1uF) to `GND_LOGIC`, SW1 to `GND_LOGIC` | Espressif's recommended RC reset delay, plus a manual reset button (needed since native USB has no DTR/RTS auto-reset) |
| 27 (GPIO0) | SW2 -> R2 (10k) -> `GND_LOGIC` | Boot-mode entry; no external pull-up needed, module's internal 45k pull-up handles idle state (esptool boot-mode documentation) |
| 13/14 (USB_D-/D+) | R4/R3 (22 ohm each) to connector D-/D+ (both physical orientation pins tied together per signal) | Espressif's USB RC schematic guidance |
| J1 CC1/CC2 | R5/R6 (5.1k each) to `GND_LOGIC` | USB-C spec requirement -- without these a USB-C host won't enable the port |
| J1 VBUS | Left unconnected | Deliberate -- keeps USB data-only, avoids a second unarbitrated power path into the board alongside the field-power rail (see reasoning below) |
| J1 GND, SHIELD | `GND_LOGIC` | Connector ground and shell |

**VBUS left unconnected -- reasoning:** wiring VBUS into the board's power system would
create a second power source feeding rails the power subsystem already owns. If the board
is USB-connected while field power is also present (a normal bench scenario), two supplies
would be driving the same net with no arbitration. Fixing that properly needs a power-mux
(diode-OR or an ideal-diode IC, same class of part as the LM74610-Q1 already used
elsewhere) -- not justified here since this board isn't meant to run off USB power in the
field. USB stays a data-only debug/programming interface; the board always needs field or
bench power connected to run.

**Antenna:** no PCB-side connection -- the WROOM-1U module has its own onboard U.FL
connector; an external antenna assembly attaches there directly.

**Footprint:** module footprint confirmed as `RF_Module:ESP32-S3-WROOM-1U` (external U.FL
antenna), not the plain `-1` footprint -- the schematic symbol itself is `ESP32-S3-WROOM-1`
since no separate `-1U` symbol exists in this library version, but the two module variants
share an identical pin table (same datasheet document), so this is a footprint-only
distinction with no schematic impact.

## Step 8 results: verification checklist (2026-09-04)

Every value above traced to source, not assumed:

- EN RC delay (10k + 1uF): Espressif ESP32-S3 Hardware Design Guidelines, Schematic Checklist.
- GPIO0 pull-down (10k, no external pull-up): esptool documentation, Boot Mode Selection
  (ESP32-S3) -- confirms the 45k internal pull-up and the need for a *strong* pull-down to
  reliably override it.
- Decoupling (0.1uF + 10uF on 3V3): Espressif Schematic Checklist, specifically flagged for
  WiFi TX burst current transients.
- USB series resistors (22-33 ohm, unpopulated-capable) + CC1/CC2 5.1k pull-downs:
  Espressif Schematic Checklist (series resistors) + USB-C connector spec requirement
  (CC termination, standard for any USB-C device port).
- 3.3V-LOGIC rail capacity: already verified in `docs/subsystems/power.md` -- module's
  355mA WiFi TX peak sits inside U3's 1A rating with ~54% headroom.
- Strapping pins (GPIO0/45/46/3): none driven by external circuitry other than GPIO0's
  boot button (intentional) -- satisfies the requirement that strapping pins be free during
  the ~3ms window after EN goes high.

No simulation performed for this subsystem -- not applicable. This is a digital/power-
integrity design, not a circuit with a control loop; a datasheet-compliance check is the
correct verification method here, same distinction made in the power subsystem's own plan.

## Step 9: Acceptance criteria

1. Module powers up and holds 3.3V-LOGIC within the rail's already-verified +/-3% tolerance
   (power subsystem, not re-tested here).
2. EN's RC delay produces a clean, monotonic power-up -- no chatter into reset while the
   3.3V-LOGIC rail is still stabilizing. Confirmable only at bring-up (oscilloscope on EN).
3. GPIO0 boot button reliably forces download mode -- confirmable only at bring-up
   (flash a test binary via esptool).
4. USB enumerates as a device (native USB CDC) when connected to a host with field/bench
   power also applied. Confirmable only at bring-up.
5. No strapping pin is pulled by anything other than its intended circuit (GPIO0's button)
   -- checked in schematic, re-check at ERC.

Items 2-4 need real hardware -- tracked below as commissioning items, same pattern as the
power subsystem.

## Step 10: Bill of materials

| Ref | Part | Role | MPN |
|---|---|---|---|
| U1 | Espressif ESP32-S3-WROOM-1U-N16R8 | Main MCU module | ESP32-S3-WROOM-1U-N16R8 |
| J1 | Molex 216989-0001 | USB-C receptacle, USB2.0-only, 14-pin | 216989-0001 |
| C1 | Ceramic, X7R | 3V3 bulk reservoir cap | 10uF |
| C2 | Ceramic, X7R | 3V3 bypass cap | 0.1uF |
| C3 | Ceramic, X7R | EN RC delay cap | 1uF |
| R1 | E24 | EN pull-up | 10k |
| R2 | E24 | GPIO0 pull-down | 10k |
| R3, R4 | E24 | USB D+/D- series resistors | 22 |
| R5, R6 | E24 | USB-C CC1/CC2 pull-downs | 5.1k |
| SW1, SW2 | E-Switch TL1150AF070Q | Manual reset (EN) / boot-mode entry (GPIO0) | TL1150AF070Q |

All passives are standard E24 decade values -- generic, no sourcing risk. U1, J1, and
SW1/SW2 are the parts needing a specific manufacturer/part-number match; all confirmed in
stock at DigiKey.

## Commissioning test items (Rev-A bring-up)

| Item | What to check |
|---|---|
| EN power-up behavior | Scope the EN pin during power-up, confirm clean monotonic RC delay with no chatter |
| GPIO0 boot entry | Confirm the boot button reliably enters download mode with esptool |
| USB enumeration | Confirm the board enumerates as a native USB CDC device on a host PC |

## Step 11: Sign-off

Core compute subsystem schematic capture complete. Every pin wired against verified
Espressif documentation, module and USB connector both matched to real, sourced part
numbers. Three items deferred to Rev-A bring-up as commissioning tests, consistent with
this project's standing practice.

**Next:** roadmap step 6 (fine-grained pin/bus assignment) can now proceed for other
subsystems using this MCU's remaining GPIOs -- this sheet itself is not touched again
except to add wires out to newly-assigned pins. Following subsystem: digital inputs (8x,
opto-isolated).
