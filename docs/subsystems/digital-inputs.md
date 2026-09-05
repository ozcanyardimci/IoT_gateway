# Digital Inputs Subsystem — Build Plan

**Status:** in progress. Requirements locked, schematic capture not started.

## Scope

8 field digital inputs, opto-isolated, on IOBOARD. Each channel: field wiring in, LTV-247
optocoupler isolation barrier, conditioned logic-level signal out to an ESP32-S3 GPIO. Not
in scope here: exact GPIO assignment (roadmap step 6, deferred the same way relay outputs
and analog I/O will be).

## Design approach

The isolation barrier itself (LTV-247) needs no separate isolated supply — the field loop's
own voltage drives the LED side; the logic side runs off the already-existing 3.3V-LOGIC
rail and returns through GND_LOGIC, with the field side returning through GND_FIELD_DI
(reserved for this in `power.md`'s grounding table). Verification here is a datasheet
compliance + resistor-math checklist, same style as core-compute — no simulation needed for
a static resistor-limited LED drive circuit.

Three design decisions were made deliberately before locking requirements (documented with
reasoning, same as every other decision in this project):

1. **Input voltage range: 10-30VDC**, matching the system's own field power input range
   (not narrowed to a fixed 24V), so the same field supply already powering other equipment
   can drive these inputs directly.
2. **Sourcing field devices, reverse-protected**: each channel accepts a sourcing (PNP-style)
   field device — the field device applies +V when active, the channel sinks current to
   GND_FIELD_DI. A single series diode per channel protects the LED if wiring is reversed
   (input just won't register until corrected — no damage). A full reverse-polarity diode
   bridge per channel (works either polarity) is a documented option for a future revision,
   not built now, to keep this first pass simpler.
3. **RC filtering + firmware debounce, not one or the other**: a single filter capacitor per
   channel forms a natural low-pass with the existing pull-up resistor, cheap insurance
   against fast EMI transients from field wiring sharing a panel with switching noise (same
   reasoning already applied to the power subsystem's TVS/Y-cap). Firmware debounce still
   handles mechanical switch-bounce timing on top of that.

## Steps

1. **Requirements** — LTV-247 real electrical specs (verified against DigiKey, LCSC, and
   the official Lite-On family datasheet), current-limiting resistor sizing across the full
   10-30V range, pull-up value, filter cap value, logic sense.
2. **Reverse-protection diode selection.**
3. **Connector selection** — 8 channels + common, real sourced part.
4. **Strapping/reserved pin cross-check** — confirm GPIO assignment later (step 6) avoids
   flash/PSRAM/strapping pins, same list as `architecture.md`.
5. **Schematic capture (KiCad)** — new sheet, `ioboard/digital_inputs.kicad_sch`, wired
   pin-by-pin, same approach as core-compute.
6. **Verification checklist.**
7. **Acceptance criteria.**
8. **Documentation & BOM.**
9. **Sign-off** — move to the next IOBOARD subsystem (relay outputs).

---

## Step 1 results: requirements (2026-09-04)

**LTV-247 verified specs** (DigiKey product page, LCSC product page, and the official
Lite-On LTV-2X7 family datasheet — cross-checked after an initial bad datasheet pull
claimed 1-channel/SIP-4 and was caught before reaching any design decision):

| Parameter | Value | Source |
|---|---|---|
| Channels per package | 4 | DigiKey, LCSC |
| Package | 16-pin SOP | DigiKey, LCSC |
| Isolation voltage | 3750 Vrms | Lite-On datasheet |
| LED forward voltage | 1.2V typ / 1.4V max at IF=20mA | Lite-On datasheet |
| LED max forward current | 50mA | Lite-On datasheet |
| CTR | 100-600% at IF=5mA, VCE=5V (no-mark option) | Lite-On datasheet |
| Output transistor Vceo(max) | 80V | Lite-On datasheet |
| Output max collector current | 50mA/channel | Lite-On datasheet |
| Collector/total power dissipation | 100mW / 170mW | Lite-On datasheet |

8 channels = **2x LTV-247** (4 channels each), matching what `power.md`'s grounding table
already assumed ("x2, 8ch").

**Current-limiting resistor (per channel, field/LED side).** Loop budget across the full
10-30V range, including the reverse-protection diode's forward drop (~0.8V typ for a small
signal diode at this current):

- Target ~12mA LED current at 30V (worst case high end) — comfortable ~4x margin under the
  50mA absolute max, and low enough that resistor power dissipation stays manageable.
- R = (30 - 1.2 - 0.8) / 0.012 = 28.0V / 12mA ~= 2333 ohm -> nearest E24 standard value: **2.4k ohm**.
- Check at 30V: I = (30 - 1.2 - 0.8) / 2400 ~= 11.7mA — 4.3x margin under 50mA max.
- Check at 10V (worst case low end, using LED Vf max 1.4V): I = (10 - 1.4 - 0.8) / 2400 ~=
  3.25mA — below the datasheet's 5mA CTR test point, so CTR isn't formally guaranteed at
  this current. Not treated as a design flaw: phototransistor CTR degrades gradually below
  the test point rather than collapsing, and the output side only needs a fraction of a mA
  to register a clean logic transition (see pull-up sizing below) — but this is exactly the
  kind of thing that can't be fully closed on paper, so it's tracked below as a Rev-A
  commissioning item, same pattern as the power subsystem's unclosed items.
- **Power dissipation:** at 30V/11.7mA, P = I^2 * R ~= 0.33W per channel. Needs a 1W-rated
  resistor (not a standard 1/8-1/4W SMD part) — noted for footprint/part selection at step 3.
  Worst case, all 8 channels active at 30V simultaneously ~= 2.6W total across these
  resistors alone — a real thermal contributor, tracked for the enclosure/layout stage.
- A constant-current diode was considered instead of a plain resistor (would remove the
  10V-vs-30V trade-off above entirely), but no specific part could be sourced and verified
  with confidence in this session (Central Semiconductor's CMJD/CMJ series datasheets don't
  publish clear current/voltage tables online) — not worth the sourcing risk for a modest
  reliability gain the resistor approach already handles with documented margin.

**Reverse-protection diode:** 1N4148 (or equivalent small-signal diode) in series with each
LED — ubiquitous, no sourcing risk, rated well beyond what's needed here (300mA / 100V vs.
~12mA / 30V required). Treated as a generic part, same as the project's standard passives.

**Output pull-up (logic side):** 10k ohm to 3.3V-LOGIC (draws ~0.33mA) — same convention
already used for EN's pull-up in core-compute. At the worst-case low LED current (~3.25mA
into the phototransistor), even a heavily-derated CTR leaves several mA of drive capability,
comfortably enough to pull the node below the pull-up's ~0.33mA and register a clean logic
low.

**Filter capacitor:** 100nF from the pulled-up node to GND_LOGIC, one per channel — forms a
natural RC low-pass with the existing 10k ohm pull-up (tau ~1ms), matched to typical
mechanical switch bounce timescales without slowing legitimate fast transitions. No separate
series resistor needed; the pull-up itself is the "R" in this filter.

**Logic sense:** active-low at the GPIO. Field device active -> LED on -> phototransistor
conducts -> pulls the node low. Firmware needs to treat LOW as the input's active state.

## Commissioning test items (Rev-A bring-up, tracked so far)

| Item | What to check | Why not closed now |
|---|---|---|
| Low-end input reliability | Confirm a clean logic-low is registered at 10V field input across all 8 channels | LED current (~3.25mA) is below the datasheet's 5mA CTR test point; margin exists but isn't formally guaranteed on paper |
| Series resistor thermal | Confirm 1W-rated resistors run within their derated limit with multiple channels active simultaneously at 30V | Real thermal behavior depends on enclosure airflow/layout, not modeled yet |

## Revision history

| Date | Change |
|---|---|
| 2026-09-04 | Scope, design approach, and step 1 requirements (LTV-247 specs, resistor sizing, pull-up, filter cap, logic sense) locked |

---

## Step 3 results: connector selection (2026-09-05)

**Phoenix Contact MC 1,5/9-ST-3,5** (MPN 1840434) — 9-position, 3.5mm pitch, 8A/160V rated,
28-16AWG wire, 1.5mm^2 screw terminals. Same family as J1 (the power input connector,
MC 1,5/2-ST-3,5), just the 9-position variant — consistent look, same crimp/tooling.
Confirmed real and in stock via Newark, cross-listed on Octopart against multiple
distributors. The 8A rating is far beyond what any channel needs (~12mA worst case) but
that's fine here — a signal connector just needs to comfortably exceed the actual current,
not match it.

9 positions = 8 signal channels + 1 shared common return (`GND_FIELD_DI`).

## Step 4 results: strapping/reserved pin cross-check (2026-09-05)

No conflict. This subsystem's outputs are generic GPIO-input signals (`architecture.md`
already lists "Digital inputs (8x) | GPIO, input" as its own bus row) — exact pin numbers
are deferred to roadmap step 6, same as every other subsystem still pending fine-grained
assignment. Nothing here requires GPIO26-32 (flash), GPIO33-37 (PSRAM), GPIO0/45/46/3
(strapping), or GPIO39/42/47/21 (nonstandard reset) — a plain digital input has no
electrical reason to need any of those specifically, so this cross-check just confirms
there's nothing to flag, not a real constraint on step 6's later assignment.

## Step 5 results: schematic capture (2026-09-05)

Wired in `hardware/kicad/ioboard/ioboard/digital_inputs.kicad_sch` — 8 channels across two
LTV-247 ICs (U7 units A-D = channels 1-4, U8 units A-D = channels 5-8), each identical:

| Signal | Net | Notes |
|---|---|---|
| Field input | `DI#_IN` (per channel) | Local label, unique per channel |
| LED anode | via R (2.4k) from `DI#_IN` | |
| LED cathode | via D (1N4148) to `GND_FIELD_DI` | Anode toward the LED, cathode toward the rail |
| Common field return | `GND_FIELD_DI` | Local label, shared across all 8 channels on this sheet |
| Phototransistor collector | via R (10k) to `3V3_LOGIC` | Hierarchical label (Input), routed to `power.kicad_sch`'s `3V3_LOGIC` output pin via an explicit wire on the root `ioboard.kicad_sch` sheet |
| Filter node | via C (100nF) to `GND_LOGIC` | Global label (shape: Input), matching the project-wide convention |
| Phototransistor emitter | direct to `GND_LOGIC` | Global label (shape: Input) |
| Field connector | J2, Phoenix Contact MC 1,5/9-ST-3,5 | Pins 1-8 = `DI1_IN`...`DI8_IN`, pin 9 = `GND_FIELD_DI` |

**Label scoping convention established/reinforced this session** (applies project-wide, not
just here): a signal that never leaves one sheet stays a **local** label (`DI#_IN`,
`GND_FIELD_DI`). A signal shared between a specific, limited set of sheets uses a
**hierarchical** label plus an explicit wire between sheet-symbol pins on the parent sheet
(`3V3_LOGIC` and every other power rail) — this makes the root sheet a real, visible map of
what connects where, not implicit plumbing. **Global** labels are reserved for the one net
used so universally that hierarchical routing would be pure repetition with no added
information — `GND_LOGIC` only, matching the precedent already set in `power.md`.

## Step 6 results: verification checklist (2026-09-05)

Every value traced to source, not assumed:

- LTV-247 electrical specs: DigiKey product page, LCSC product page, and the official
  Lite-On LTV-2X7 family datasheet (cross-checked after an initial bad datasheet pull was
  caught and corrected before it reached any design decision).
- Current-limiting resistor (2.4k, 1W): hand-calculated across the full 10-30V input range,
  including the reverse-protection diode's forward drop — 4.3x margin under LED max current
  at the worst-case high end; low-end margin against the CTR test point documented and
  tracked as a Rev-A commissioning item rather than glossed over.
- Reverse-protection diode (1N4148): standard, ubiquitous part, rated far beyond what's
  needed (300mA/100V vs. ~12mA/30V required) — no sourcing risk.
- Pull-up (10k) and filter cap (100nF): sized against the LTV-247's real drive capability
  with wide margin, same reasoning style as EN's pull-up in core-compute.
- Connector (Phoenix Contact MC 1,5/9-ST-3,5): confirmed real and in stock across multiple
  independent distributor listings.
- Schematic verified pin-by-pin against the actual placed KiCad symbols at each step, not
  assumed from a datasheet pinout diagram (which couldn't be reliably sourced for this
  part) — every channel's pin numbers, label spelling, and diode orientation checked against
  screenshots before moving to the next channel.

## Step 7: Acceptance criteria

1. Each channel accepts a sourcing (PNP-style) field device across the full 10-30VDC range
   without exceeding the LTV-247's LED current rating at any point in that range.
2. Reverse field wiring on any channel is blocked by that channel's protection diode with no
   damage to the LED.
3. Each channel's logic output registers a clean, debounced active-low signal at the GPIO
   node (active-low: field device active -> LED on -> phototransistor conducts -> node pulled
   low).
4. `GND_FIELD_DI` (field-side) has no direct DC path to `GND_LOGIC` (logic-side) — isolation
   boundary intact, same requirement style as the power subsystem's ground-domain checks.
5. All 8 channels are electrically independent of one another (no cross-channel coupling
   through the shared `GND_FIELD_DI` or `3V3_LOGIC`/`GND_LOGIC` rails beyond normal common-
   return behavior).

Items 1, 3, and 4 need real hardware to close out fully — Rev-A bring-up checks, listed
below alongside the two items already identified during resistor sizing.

## Step 8: Bill of materials

| Ref | Part | Role | MPN |
|---|---|---|---|
| U7, U8 | Lite-On LTV-247 | 4-channel opto-isolator (x2 for 8ch) | LTV-247 |
| J2 | Phoenix Contact MC 1,5/9-ST-3,5 | 8-channel + common field connector | 1840434 |
| R10, R12, R15, R14, R19, R18, R23, R22 | E24 | Current-limiting resistor (per channel), 1W | 2.4k |
| R11, R13, R17, R16, R21, R20, R25, R24 | E24 | Pull-up (per channel) | 10k |
| D1-D8 | Generic small-signal diode | Reverse-protection (per channel) | 1N4148 |
| C13, C14, C16, C15, C18, C17, C20, C19 | Ceramic, X7R | Filter cap (per channel) | 100nF |

U7/U8 and J2 are the only parts needing a specific manufacturer/part-number match; both
confirmed in stock. Passives are standard E24/generic values, no sourcing risk, same
precedent as every other subsystem so far.

## Commissioning test items (Rev-A bring-up)

| Item | What to check | Why not closed now |
|---|---|---|
| Low-end input reliability | Confirm a clean logic-low is registered at 10V field input across all 8 channels | LED current (~3.25mA) is below the datasheet's 5mA CTR test point; margin exists but isn't formally guaranteed on paper |
| Series resistor thermal | Confirm 1W-rated resistors run within their derated limit with multiple channels active simultaneously at 30V | Real thermal behavior depends on enclosure airflow/layout, not modeled yet |
| Isolation boundary | Confirm no DC path between `GND_FIELD_DI` and `GND_LOGIC` with a meter, post-assembly | Schematic-level check only until real hardware exists |

## Step 9: Sign-off

Digital inputs subsystem schematic capture complete. All 8 channels wired pin-by-pin against
real component pinouts (verified from placed symbols, not assumed from an unreliable
datasheet pinout diagram), every part sourced to a real, in-stock manufacturer part number
except standard passives. Three items deferred to Rev-A bring-up as commissioning tests,
consistent with this project's standing practice. This session also established a clearer,
project-wide label-scoping convention (local vs. hierarchical vs. global) applied here and
intended to guide every remaining subsystem.

**Next:** relay outputs (4x) — roadmap step 4, next in the `docs/subsystems/` order.

## Revision history

| Date | Change |
|---|---|
| 2026-09-04 | Scope, design approach, and step 1 requirements (LTV-247 specs, resistor sizing, pull-up, filter cap, logic sense) locked |
| 2026-09-05 | Connector selected, strapping-pin cross-check done, full schematic capture (8 channels + J2), verification checklist, acceptance criteria, BOM, sign-off |
