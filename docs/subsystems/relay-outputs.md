# Relay Outputs Subsystem — Build Plan

**Status:** in progress. Requirements locked, schematic capture not started.

## Scope

4 relay outputs on IOBOARD. Each channel: an ESP32-S3 GPIO switches a transistor driver
stage, which switches the relay coil (`5V_RELAY` rail, already sized for this in
`power.md`'s load budget), and the relay's mechanical contacts switch an external load
supplied by the user's own installation. Not in scope here: exact GPIO assignment (roadmap
step 6, same deferral as every other subsystem), and any load-side snubbing (see reasoning
below).

## Design approach

The relay itself was already selected during power subsystem work — Panasonic ALDP105 — and
its coil current (40mA) was already budgeted into the `5V_RELAY` rail. Re-verified fresh
against Panasonic's own product page rather than trusted from memory (see Step 1 below).

A GPIO cannot drive this coil directly: it's beyond safe GPIO output current, and a coil is
inductive, so switching it directly would expose the GPIO to an unprotected flyback voltage
spike on turn-off. Every channel therefore needs a small transistor driver stage between the
GPIO and the coil — a standard, well-understood circuit, not a novel design.

Four decisions were made deliberately before locking requirements, documented with reasoning
the same as every other decision in this project:

1. **Transistor driver, generic NPN, generously overdriven base.** MMBT3904 chosen for the
   same reason 1N4148 and E24 passives get reused throughout this project: ubiquitous,
   multi-sourced, real verified ratings well beyond what's needed here. Base current is sized
   for a comfortable saturation margin rather than the bare minimum (see Step 1 math) —
   deliberate given the contacts on the other side of this relay can be switching real,
   sometimes hazardous loads; an unreliable driver stage is not somewhere to cut margin.
2. **Defined-off state at boot.** A base pull-down resistor holds each channel off if its
   GPIO is floating during boot/reset, before firmware takes control. Same reasoning as
   digital-inputs' filter cap and core-compute's GPIO0 pull-down: don't leave a
   safety-relevant state undefined during power-up just because it usually works out fine.
3. **Flyback protection reuses 1N4148.** Already proven and stocked in this project; its
   200-300mA continuous rating comfortably covers a 40mA coil's turn-off transient. No new
   part introduced for a requirement an existing part already covers with margin.
4. **No load-side snubbing built in.** What's wired to each relay's contacts is unknown at
   design time — resistive, inductive, AC, or DC, decided entirely by the user's own
   installation. Snubbing/protection for that load is standard practice to document as a
   usage note, not to speculatively build onto the board. Same reasoning as core-compute's
   USB VBUS decision: don't add scope for a requirement that doesn't exist yet.

**Known limitation, flagged rather than silently absorbed:** the field connector selected in
Step 2 is rated 160V, while the ALDP105's contacts are themselves rated up to 277VAC/30VDC.
The connector — not the relay — is the practical ceiling on what this board can switch. This
is treated as an acceptable default (160V/8A per channel still covers most industrial control
loads: contactor coils, solenoids, indicator lights, low-voltage motors, 24-120VAC control
circuits) rather than a defect, on the same "don't over-provision for an unstated
requirement" reasoning as the snubbing decision above. If mains-adjacent switching
(208-277VAC) is ever a real requirement, the fix is a straight connector swap to a
higher-voltage-rated part in the same footprint family — it does not change the schematic,
driver stage, or relay selection at all.

## Steps

1. **Driver stage** — real transistor specs, base resistor sizing with shown margin, flyback
   diode, pull-down.
2. **Connector selection** — output terminal arrangement, real sourced part.
3. **Strapping/reserved pin cross-check** — same list as `architecture.md`; genuinely
   actionable once roadmap step 6 assigns exact GPIOs, not before.
4. **Schematic capture (KiCad)** — new sheet, `ioboard/relay_outputs.kicad_sch`, wired
   pin-by-pin, same approach as digital-inputs (verify real pin numbers live against the
   placed symbol, since no numbered pinout diagram could be sourced for the ALDP105 — same
   situation as the LTV-247).
5. **Verification checklist.**
6. **Acceptance criteria.**
7. **Documentation & BOM.**
8. **Sign-off** — move to the next IOBOARD subsystem (analog I/O).

---

## Step 1 results: driver stage (2026-09-05)

**Panasonic ALDP105 verified specs** (Panasonic's own industry product page, re-checked
today rather than trusted from the earlier power-subsystem note):

| Parameter | Value |
|---|---|
| Coil rated voltage | 5V DC |
| Coil resistance | 125Ω ± 10% |
| Coil rated current | 40.0mA ± 10% |
| Operate voltage (max) | 75% of rated (≤3.75V) |
| Release voltage (min) | 5% of rated (≥0.25V) |
| Contact configuration | 1 Form A (SPST-NO) |
| Contact rating | 5A AC / 3A DC, resistive |
| Max switching voltage | 277VAC / 30VDC |
| Max switching power | 1385VA / 90W |
| Package | PC-board (through-hole), "slim type" |

No numbered pinout diagram exists in any datasheet found — only a functional
COIL/COM/NO "bottom view" schematic (Future Electronics PDF, already in the datasheets
list). Pin numbers get verified live against the placed KiCad symbol at Step 4, same
resolution used for the LTV-247.

**Driver transistor: MMBT3904** (Nexperia's own datasheet, fetched fresh today):

| Parameter | Value |
|---|---|
| Vceo max | 40V |
| Ic max (continuous) | 200mA |
| hFE min at Ic=10mA | 100 (max 300) |
| hFE min at Ic=100mA | 30 |
| Vce(sat) max | 200mV @ 10mA / 300mV @ 50mA |
| Package | SOT-23 |

**Base resistor sizing.** Our coil current (40mA) falls between the datasheet's two
guaranteed-hFE test points (10mA and 100mA) — the conservative choice is to design against
the lower of the two guaranteed values (hFE_min = 30 at the 100mA point) rather than
interpolate:

- Bare-minimum base current for saturation: Ib = Ic / hFE_min = 40mA / 30 ≈ 1.33mA.
- Target a comfortable overdrive margin rather than the bare minimum — chosen: ~5mA.
- R = (Vgpio - Vbe) / Ib = (3.3V - 0.7V) / 5mA = 520Ω → nearest E24 standard value: **510Ω**.
- Check: Ib = 2.6V / 510Ω ≈ 5.1mA. Sustainable Ic at this Ib and hFE_min = 30 is
  5.1mA × 30 ≈ 153mA — a 3.8x margin over the 40mA actually needed, i.e. solidly saturated
  across temperature and part-to-part variation, not marginal.
- GPIO loading: ~5.1mA is trivial for an ESP32-S3 GPIO (comfortably under its rated drive
  current) — not re-verified against Espressif's datasheet this session, flagged as the one
  number here carried from general knowledge rather than a fresh source check.

**Base pull-down: 10kΩ**, same value already used project-wide (EN pull-up, GPIO0
pull-down, digital-input pull-ups). Sits directly across the base-emitter junction; because
that junction clamps near 0.7V rather than behaving as a linear resistor, only a negligible
~70µA is diverted through the pull-down when the GPIO drives the base high — it does not
meaningfully rob saturation margin, and it holds the base (and therefore the relay) off
whenever the GPIO is floating.

**Flyback diode: 1N4148**, across the coil, cathode to `5V_RELAY`, anode to the
collector/coil-low node — absorbs the coil's stored energy on turn-off. Same part already
qualified and stocked for the digital-inputs subsystem; its 200-300mA rating leaves large
margin over a 40mA coil's transient.

**Per-channel topology:**

- GPIO → 510Ω → transistor base
- Base → 10kΩ → `GND_LOGIC` (pull-down)
- Transistor emitter → `GND_LOGIC`
- Transistor collector → coil low side
- Coil high side → `5V_RELAY`
- 1N4148 flyback diode across the coil (cathode to `5V_RELAY`, anode to collector node)
- Relay contacts (COM/NO) → field connector, no on-board ground reference (pure isolated
  mechanical switch)

## Step 2 results: connector (2026-09-05)

4 relays are broken out fully independent — 8 positions total (COM + NO per channel) rather
than a shared common bus — so each relay can switch a completely unrelated circuit. This
matches a general-purpose industrial I/O card default; the alternative (5 positions, 1 shared
common + 4× NO) would only make sense if all 4 loads were known in advance to share one
supply rail, which isn't the case here.

**Phoenix Contact MC 1,5/8-ST-3,5** (MPN 1840421) — same connector family already used for
J1 (power input) and J2 (digital inputs): 3.5mm pitch, 8A/160V, 28-16AWG (1.5mm²) screw
terminals, through-hole pluggable. Confirmed in stock via Newark and Farnell listings.

See the "Known limitation" note above regarding this connector's 160V rating vs. the
relay's own 277VAC contact rating.

## Step 3: strapping/reserved pin cross-check

Not yet actionable. `architecture.md` defers exact GPIO assignment to roadmap step 6 for
every subsystem, including this one — no specific GPIO has been assigned to any relay
channel yet, so there is nothing yet to check for conflicts against the reserved list
(GPIO26-32 flash, GPIO33-37 PSRAM, GPIO0/45/46/3 strapping, GPIO39/42/47/21 nonstandard
reset). Carried forward as a real step, done once step 6 happens.

## Revision history

| Date | Change |
|---|---|
| 2026-09-05 | Scope, design approach, driver-stage math, and connector selection locked |
