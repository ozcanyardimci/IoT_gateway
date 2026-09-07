# Analog I/O Subsystem — Build Plan

**Status:** Steps 1 (front-end + isolation-crossing design), 3 (connector), and 5 (schematic
capture) locked and verified — schematic capture verified pin-by-pin against the saved
`analog_io.kicad_sch` file, not by self-report. Step 6 (verification checklist) is next.

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
  above. `15V_ANALOG_ISO` is already brought out as a dangling root-sheet pin on the
  `analog_io` sheet symbol, ready to be wired once that rail exists.

### Known headroom (not a defect)

Two LM2904 chips (4 amps total) are needed for 3 amp-slots (2 input buffers + 1 output gain
stage) — one half-amp is spare. Flagged as available headroom for a future channel or
diagnostic use, not wasted design.

## Step 3 results: connector (2026-09-07)

**Phoenix Contact MC 1,5/4-ST-3,5** (MPN 1840382) — same family already used for J1 (power
input), J2 (digital inputs), and J3 (relay outputs): 3.5mm pitch, 8A/160V, 28-16AWG (1.5mm²)
screw terminals, through-hole pluggable, 4 positions. Confirmed in stock via Newark/TME
listings.

4 positions, not 6: AI1, AI2, and AO each get one signal pin, sharing **one common
GND_ANALOG_ISO return** rather than a dedicated return per channel.

- This is a deliberate difference from the relay-outputs connector, which gave every
  channel a fully independent COM+NO pair specifically because relay contacts have no
  shared reference and could be switching unrelated circuits.
- The analog channels don't have that problem — all three already share one physical
  isolated ground plane (GND_ANALOG_ISO) by design (see the isolation-crossing decision
  above). Giving each channel its own return terminal would still land on the same net on
  the PCB; it would add connector pins and cost without adding isolation or noise
  rejection that a shared return doesn't already provide.
- 8A/160V is heavy overkill for signal-level current (tens of mA at most) — kept anyway for
  BOM/part-family consistency with J1/J2/J3, same reasoning already used project-wide for
  reusing one connector family across subsystems.

## Step 5 results: schematic capture (2026-09-07)

`ioboard/analog_io.kicad_sch` built and verified pin-by-pin against the saved file content
(not by self-report) across every component, wire, and label on the sheet.

**Components placed:**

- **U9** ADS1115IDGSR (ADC) — AIN0/AIN1 from the two input buffers, VDD/GND on the
  isolated 3.3V/GND rails, ADDR strapped to GND_ANALOG_ISO (address 0x48), SDA/SCL on the
  isolated I2C bus. AIN2/AIN3 and ALERT/RDY left unconnected (spare/deferred).
- **U10** ISO1540 (I2C isolator) — VCC1/GND1 on the isolated rails, SDA1/SCL1 on the
  isolated bus shared with U9/U11; VCC2 on 3V3_LOGIC, GND2 on the global GND_LOGIC net;
  SDA2/SCL2 given local labels `SDA_ISO`/`SCL_ISO` (plain local labels, matching the
  RELAY#_CTRL convention already established in `relay_outputs.kicad_sch` — not
  hierarchical labels, since these don't need a root-sheet pin).
- **U11** MCP4725A0T-E/CH (DAC) — A0 strapped to GND_ANALOG_ISO (address 0x60, no
  collision with the ADC's 0x48), VDD/VSS on the isolated rails, SCL/SDA on the isolated
  bus, VOUT into the output gain stage.
- **U12** — physical LM2904 package #1: both amp units used as unity-gain input buffers
  (channel 1 -> U9 AIN0, channel 2 -> U9 AIN1), powered from 3V3_ANALOG_ISO / GND_ANALOG_ISO.
- **U13** — physical LM2904 package #2: one amp unit used as the output non-inverting gain
  stage (gain 3.004, per Step 1 math), powered from 15V_ANALOG_ISO / GND_ANALOG_ISO. Second
  unit intentionally left unplaced — the spare headroom flagged in Step 1.
- **D13, D14** — SMBJ15CA TVS, one per input channel, across each channel's signal node to
  GND_ANALOG_ISO, ahead of the divider.
- **R_top1/R_bottom1** (channel 1, AI1) and **R_top2/R_bottom2** (channel 2, AI2) — the
  100k/11.0k stuffing-option dividers from Step 1, reference designators confirmed unique
  and channel-matched (no naming collision) via direct grep of the saved file.
- **R_in (49.9k) / R_f (100k)** — output-stage feedback network. Verified the "-" pin,
  R_in's GND-side leg, and R_f's output-side leg all meet at one shared node (the actual
  feedback-loop requirement for a non-inverting stage) — an earlier wiring pass had this
  wrong (R_f tied to GND instead of to the "-"/R_in node, which breaks feedback entirely)
  and was caught and corrected during capture.
- **J4** — Phoenix Contact MC 1,5/4-ST-3,5 (Step 3's connector): pin 1 = AI1, pin 2 = AI2,
  pin 3 = AO, pin 4 = GND_ANALOG_ISO.

**Nets/labels confirmed:** `AI1`, `AI2`, `AO` (local labels tying each front-end node to its
connector pin), `SDA_ISO`/`SCL_ISO` (local labels, logic-side I2C), multiple
`GND_ANALOG_ISO` and `3V3_ANALOG_ISO` hierarchical-label instances, one `15V_ANALOG_ISO`
hierarchical label (intentionally left dangling — no rail exists yet, see below), one
`3V3_LOGIC` hierarchical label, one `GND_LOGIC` global label. No accidental bridging found
between the GND_ANALOG_ISO and GND_LOGIC domains anywhere on the sheet — the isolation
barrier (U10) is the only crossing point, as designed.

**Root-sheet integration:** the `analog_io` sheet symbol's 4 pins (`3V3_ANALOG_ISO`,
`GND_ANALOG_ISO`, `15V_ANALOG_ISO`, `3V3_LOGIC`) confirmed present and correctly wired on
`ioboard.kicad_sch`; `15V_ANALOG_ISO` is deliberately left unwired at the root level until
the power subsystem adds that rail (Step 2 of this doc's own plan, still deferred).

## Step 6 results: verification checklist (2026-09-07, schematic-level; in progress)

Schematic-level checks completed as part of capture, before any physical hardware exists:

- Every IC pin (power, signal, address-strap) traced to its correct net — done manually,
  pin-by-pin, verified against the saved file rather than the KiCad canvas view.
- I2C addresses confirmed distinct: ADS1115 = 0x48 (ADDR->GND), MCP4725 = 0x60 (A0->GND) —
  no collision on the shared isolated bus.
- Ground-domain isolation confirmed: GND_ANALOG_ISO and GND_LOGIC never share a net anywhere
  on the sheet except through U10 (ISO1540) — the isolation barrier is intact in the
  schematic, not just intended.
- Feedback-loop topology on the output gain stage confirmed correct (see Step 5 above) —
  this is the one point on the sheet where a wiring mistake would have been electrically
  silent until power-up, so it got the closest look.
- Resistor reference designators confirmed unique and unambiguous (R_top1/R_bottom1 = AI1,
  R_top2/R_bottom2 = AI2) after a rename caught during the sheet-wide audit.

Still open (needs either KiCad's own tooling or physical hardware):

- **Run KiCad's ERC (Electrical Rules Check)** on `analog_io.kicad_sch` — an independent,
  tool-based cross-check on top of the manual pin tracing above (catches things like
  unpowered pins, conflicting driver pins, or unrouted labels that manual review can miss).
  Not yet run.
- Physical checks deferred to Rev-A hardware bring-up: rail presence and isolation
  (megohmmeter check between GND_ANALOG_ISO and GND_LOGIC), I2C bus scan confirming both
  devices ACK at their expected addresses, AI1/AI2 functional test against a known input
  voltage, AO functional test against a commanded DAC code. None of this is possible before
  PCB fabrication; listed here so it isn't lost before Step 7 (acceptance criteria) formalizes
  pass/fail thresholds for each.

## Steps

1. **Front-end + isolation-crossing design** — real part selection, front-end math,
   isolation-crossing decision — **DONE (this doc)**.
2. **New rail spec** — written into `power.md`, not duplicated here — deferred until this
   subsystem is otherwise closed out, per the user's own sequencing call.
3. **Connector selection** — **DONE (this doc)**.
4. **Strapping/reserved pin cross-check** — deferred to roadmap step 6, same as every other
   subsystem.
5. **Schematic capture (KiCad)** — new sheet, `ioboard/analog_io.kicad_sch` — **DONE (this
   doc)**.
6. **Verification checklist** — in progress (this doc); ERC pending.
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
| 2026-09-07 | Step 3 connector locked: Phoenix Contact MC 1,5/4-ST-3,5, 4 positions
  (AI1/AI2/AO + shared GND_ANALOG_ISO return), same family as J1/J2/J3. |
| 2026-09-07 | Step 5 schematic capture done: `analog_io.kicad_sch` built and verified
  pin-by-pin (U9 ADS1115, U10 ISO1540, U11 MCP4725, U12/U13 LM2904 x2 packages, D13/D14
  SMBJ15CA, front-end dividers, output gain stage, J4). One real wiring bug caught and fixed
  (output-stage feedback network wired to break the loop) and one naming ambiguity caught and
  fixed (channel-1/channel-2 resistor reference designators swapped). Root-sheet integration
  confirmed on `ioboard.kicad_sch`. Step 6 verification checklist started: schematic-level
  checks (address collisions, ground-domain isolation, feedback topology) done; ERC and
  physical bring-up checks still open. |
