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
