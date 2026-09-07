# Analog I/O Subsystem — Build Plan

**Status:** Step 1 (front-end + isolation-crossing design) locked and verified against real
datasheets. Schematic capture not yet started.

## Scope

2 analog inputs (0-10V / 4-20mA, per-channel stuffing option) + 1 analog output (0-10V), all
isolated from GND_LOGIC via the GND_ANALOG_ISO domain already established in the power
subsystem. Not in scope here: exact I2C address / GPIO assignment (roadmap step 6), PCB
layout, and the new power rail this subsystem requires (that's a power-subsystem change —
see "Power subsystem impact" below; it gets added to `power.md` once this doc is locked).

## Design approach

Four decisions, made deliberately before part selection, documented with reasoning the same
as every other decision in this project:

1. **Channel count: 2 analog inputs + 1 analog output.** A representative general-purpose
   mix for a small industrial gateway — enough for a couple of sensor inputs and one control
   setpoint output — without over-provisioning channel count the way the digital I/O
   subsystems deliberately didn't either (4 relay, 8 DI).
2. **Input type: both 0-10V and 4-20mA per channel, via a stuffing option.** These are the
   two dominant industrial analog signal standards. Supporting both from one PCB layout
   (populate a voltage divider for 0-10V mode, or a burden resistor for 4-20mA mode — never
   both) avoids needing two separate board SKUs for what is otherwise the same circuit.
3. **Isolation: isolated ADC + digital isolator, extended to the output too.** The power
   subsystem already added isolation to the analog input as "a deliberate deviation... worth
   the small BOM cost" (see `power.md`, grounding & isolation architecture). Leaving the
   output unisolated right next to an isolated input would reintroduce the exact ground-loop
   exposure that isolation was added to avoid in the first place — so this locks the output
   into the same isolated domain rather than treating it as a separate island.
4. **Output implementation: a real DAC + op-amp gain stage, not PWM+RC filter.** Same
   "real ICs over workarounds" reasoning already used for the relay driver stage and the
   digital-input opto-isolators. The ESP32-S3 has no internal DAC peripheral (see
   `architecture.md`'s fixed MCU constraints); a PWM+filter approach trades a slow filter
   time constant against read-back latency and ripple, which is a worse tradeoff for an
   analog control output than a dedicated 12-bit DAC.

## Power subsystem impact (flagged now, applied later)

Checking the existing rail budget (`power.md`) against decision 4 surfaced a real gap: no
rail in this design exceeds 5V, but a true 0-10V output needs headroom above that (see
Step 1 math below for why). This subsystem's design assumes **one new isolated ~15V rail**,
sourced from the already-regulated 5V-RELAY rail rather than a new tap on raw field power.
That rail addition (module selection, load budget entry, fuse-budget check) is written up in
`power.md`, not here, once this doc is locked — same pattern as relay-outputs.md consuming
the already-sized 5V_RELAY rail without redefining it.

---

## Step 1 results: front-end + isolation-crossing design (2026-09-07)

### Isolated-side ADC: TI ADS1115

| Parameter | Value |
|---|---|
| Resolution | 16-bit |
| Channels | 4 single-ended (2 used; headroom for future expansion) |
| Supply range | 2.0V-5.5V — fits the existing 3.3V-ANALOG-ISO rail directly |
| PGA full-scale options | ±6.144V / ±4.096V / ±2.048V / ±1.024V / ±0.512V / ±0.256V |
| Absolute max input | GND-0.3V to VDD+0.3V — **the physical pin voltage is bounded by
  VDD regardless of PGA setting**; a wider PGA range doesn't allow a wider physical input
  swing on a single 3.3V supply |
| Data rate | 8-860 SPS, field-selectable |
| I2C addressing | 4 addresses via ADDR pin (GND/VDD/SDA/SCL) |

### Isolation crossing: TI ISO1540 (not ISO1541)

| Parameter | Value |
|---|---|
| Channels | 2, **both bidirectional** (SDA and SCL) |
| Supply range | 3V-5.5V each side |
| Isolation | 2500 Vrms continuous, 4242 Vpk transient |
| Max data rate | 1 MHz (Fast-mode Plus) |

ISO1541 was ruled out specifically: its SCL channel is unidirectional (input-only on one
side), which can't support I2C clock-stretching. Since this isolator carries both the ADC
and the DAC on the same bus, the fully-bidirectional ISO1540 is the correct choice, not just
the safer one.

### Isolated-side DAC: Microchip MCP4725

| Parameter | Value |
|---|---|
| Resolution | 12-bit |
| Supply range | 2.7V-5.5V |
| Output range | Rail-to-rail, **ratiometric to VDD** — output cannot exceed VDD |
| Settling time | 6 us typical |
| I2C addressing | 8 addresses via A0 pin |

Because the DAC's own output is capped at VDD, and VDD's max rating (5.5V) is still well
short of 10V, the DAC runs on the **same 3.3V-ANALOG-ISO rail as the ADC** — one fewer
voltage domain to manage — and a separate gain stage (below) does the 3.3V-to-10V lift. This
is also the real gap flagged above: that gain stage needs a supply rail with more headroom
than 3.3V provides.

### Buffer / gain-stage op-amp: LM2904 (already qualified — power.md's own analog-stage line
item)

Two physical LM2904 instances are needed: one covers both input-channel buffers (2 of 2
amps used), the second covers the output gain stage (1 of 2 amps used — second half spare).

**Real-datasheet catch, caught before it became a schematic bug:** LM2904's input
common-mode range on a single supply only reaches to about (V+ minus 1.5-2V), not the full
rail — it is not a rail-to-rail-input part. On the 3.3V-ANALOG-ISO rail that ceiling is
roughly **1.3-1.8V**, not 3.3V. Every input-side divider/burden-resistor value below is
chosen to respect that ceiling with margin — an earlier version of this design (before
checking the LM2904 datasheet directly) assumed signals could swing up to ~3V into the
buffer, which would have been out of the op-amp's linear input range on this rail.

### Input front end (per channel, stuffing option — populate one path, not both)

**0-10V mode:** resistive divider, R_top = 100k / R_bottom = 11.0k (both real E96 values).

- Ratio = 11.0k / (100k + 11.0k) = 0.0991
- 10V input -> 0.991V at the divider tap — **24% margin below the 1.3V worst-case LM2904
  common-mode ceiling**, not just below the nominal 1.8V figure.

**4-20mA mode:** burden resistor, 49.9Ω (E96 value). Recommend a 0.1%-tolerance metal-film
part here specifically — its exact value *is* the current-to-voltage calibration factor for
this channel, not just a bias resistor, so tolerance matters more here than elsewhere in
this project.

- 4mA -> 0.200V, 20mA -> 0.996V — lands in the same safe range as the 0-10V mode, so both
  stuffing options can share one ADC PGA setting.

**Overvoltage protection:** SMBJ15CA bidirectional TVS (15V standoff, 16.7-18.5V breakdown,
24.4V max clamping) across each input channel, ahead of the divider/burden resistor. Clears
the 0-10V/4-20mA operating range with margin (standoff is 50% above the 10V max normal
signal) while clamping miswiring or surge events before they reach the divider or the
op-amp.

**Buffer stage:** divider/burden node -> LM2904 unity-gain follower -> ADS1115 input pin.
Gives the ADC a low-impedance, protected source and isolates it from the ADC's own
sampling-capacitor charge-kickback — the same role this op-amp was already budgeted for in
`power.md`.

**ADC PGA setting:** ±2.048V (the default range) comfortably covers the ~1V max buffered
signal with margin, giving 62.5 uV/LSB — referred back to the original 0-10V/4-20mA signal,
that's about 630 uV/LSB (0-10V mode) or 12.6 uA/LSB (4-20mA mode). Both are well beyond what
a general-purpose industrial input needs; no PGA switching between modes required.

### Output stage

- MCP4725 on 3.3V-ANALOG-ISO: 12-bit, ~805 uV/LSB at the DAC pin (0 to ~3.3V).
- LM2904 (second instance), non-inverting gain stage: R_in = 49.9k, R_f = 100k (both real
  E96 values) -> gain = 1 + (100k/49.9k) = 3.004.
- DAC full-scale (3.3V) x 3.004 = **9.91V** at the output — deliberately landing a hair
  under the nominal 10.00V full scale rather than over it, so normal operation never clips
  even with component tolerance stacking.
- Output resolution: 12-bit DAC resolution carries through gain unchanged in LSB count —
  4096 counts over the output span, same as the DAC's own native resolution.

**New rail requirement:** LM2904's output only swings to (V+ minus ~1.5-2V) — to reach
~9.91V it needs a supply of at least ~11.5-12V, and this project doesn't cut margin that
close (see the relay-outputs base-resistor margin, the connector-vs-relay-rating margin,
etc.) — so the target is a **15V** rail, giving (15 - 2) = 13V worst-case max swing against
a 9.91V target: ~30% margin, not a bare-minimum fit.

| Parameter | Value |
|---|---|
| Candidate part | Recom R05P215S |
| Input | 5V (from the existing, already-budgeted 5V-RELAY rail — not a new raw-field tap) |
| Output | 15V |
| Rated power/current | 2W / 133mA |
| Isolation | Same Econoline family class as the already-used R1SX-3.33.3-R; exact kVDC
  figure for this specific part not independently re-verified this session — confirm
  against the datasheet PDF directly before BOM lock, same "not yet re-verified" flag used
  elsewhere in `hardware/datasheets/README.md` |

**Load check:** this rail only feeds the output gain stage's LM2904 half — quiescent draw
~0.35mA, plus output load current (a downstream analog input is typically >=100k ohm
impedance, so <=100 uA at 9.91V). Total draw is comfortably under 1mA against a 133mA-rated
module — heavy margin, consistent with this project's "proven module with margin, not a
custom design sized to the bare load" pattern for every other rail.

### I2C bus extension

ADS1115 and MCP4725 share one I2C bus on the isolated side, crossing to GND_LOGIC through
the single ISO1540. This joins the *kind* of bus already used for the status-LED expander
(architecture.md's I2C GPIO expander) — same bus type, a physically separate isolated
segment, addresses distinguished by each chip's own address pins. No new bus type
introduced.

### Still open / deferred

- Exact I2C addresses — roadmap step 6, same deferral as GPIO assignment project-wide.
- ADS1115 / ISO1540 / MCP4725 / SMBJ15CA footprints not yet verified — same deferred-to-
  pre-merge policy as every other subsystem.
- New 15V rail's addition to `power.md` (module selection table, load budget, fuse-budget
  check) — written up separately once this doc is locked, per "Power subsystem impact"
  above.
- Schematic capture (Step 2 onward).

### Known headroom (not a defect)

Two LM2904 chips (4 amps total) are needed for 3 amp-slots (2 input buffers + 1 output gain
stage) — one half-amp is spare. Flagged as available headroom for a future channel or
diagnostic use, not wasted design.

## Steps

1. **Front-end + isolation-crossing design** — real part selection, front-end math,
   isolation-crossing decision — **DONE (this doc)**.
2. **New rail spec** — written into `power.md`, not duplicated here.
3. **Connector selection** — real sourced field connector, TBD.
4. **Strapping/reserved pin cross-check** — deferred to roadmap step 6, same as every other
   subsystem.
5. **Schematic capture (KiCad)** — new sheet, `ioboard/analog_io.kicad_sch`.
6. **Verification checklist.**
7. **Acceptance criteria.**
8. **Documentation & BOM.**
9. **Sign-off** — move to the next IOBOARD subsystem (RS485).

## Revision history

| Date | Change |
|---|---|
| 2026-09-07 | Scope and 4 design decisions locked (2 AI + 1 AO, 0-10V/4-20mA per-channel
  stuffing option, isolation extended to output, DAC-based output). Step 1 front-end and
  isolation-crossing design done: ADS1115, ISO1540, MCP4725, LM2904 (x2 instances),
  SMBJ15CA selected with real datasheet math. New isolated 15V rail requirement identified
  — to be added to `power.md`. |
