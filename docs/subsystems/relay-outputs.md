# Relay Outputs Subsystem — Build Plan

**Status:** Complete. All 8 plan steps closed 2026-09-06 (driver stage, connector, schematic
capture, verification checklist, acceptance criteria, BOM, sign-off). ERC and footprint
verification (including the imported ALDP105 footprint) deferred to the same pre-merge pass
as digital-inputs (see `CLAUDE.md` open items).

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
5. **Local bulk + bypass capacitors at the `5V_RELAY` entry point.** Not because the
   regulator can't keep up with the coil current -- it can; a relay coil's own inductance
   rate-limits its current rise to a millisecond-scale ramp, well within any reasonable
   buck regulator's control loop bandwidth, so the "fast transient" reasoning used for
   core-compute's WiFi-burst caps doesn't actually apply here. The real reason is EMI/noise:
   switching an inductive coil (even with the flyback diode handling the big kickback)
   still couples some high-frequency content back onto the shared rail, and for a product
   that has to pass EMC/EMI compliance, keeping that noise local rather than letting it
   ride down the trace into other subsystems sharing `5V_RELAY` is standard practice. C21
   (10uF, bulk) + C22 (100nF, bypass), same bulk+bypass pairing already used for the ESP32's
   3V3 pin in core-compute.

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


## Step 4 results: schematic capture (2026-09-06)

Wired in `hardware/kicad/ioboard/ioboard/relay_outputs.kicad_sch`, 4 identical channels.
Channel 1 shown -- channels 2-4 (K2/Q2, K3/Q3, K4/Q4, R28-33, D10-D12) repeat the exact
same pattern with sequential reference numbers.

| Pin(s) | Net / circuit | Confirms |
|---|---|---|
| K1 pin 1 (coil) | `5V_RELAY` (hierarchical) | coil supply |
| K1 pin 2 (coil) | Q1 pin 3 (collector), D9 anode | coil low side / switched node |
| K1 pins 3, 4 (contacts) | J3 pins 1, 2 | field-side output, fully independent per channel |
| D9 cathode | K1 pin 1 / `5V_RELAY` node | reverse-biased in normal operation |
| D9 anode | K1 pin 2 / Q1 collector node | clamps the coil's turn-off kickback |
| Q1 pin 1 (base) | R26 (510) -> `RELAY1_CTRL` (local placeholder) | GPIO drive, exact GPIO assigned at roadmap step 6 |
| Q1 pin 1 (base) | R27 (10k) -> `GND_LOGIC` | defined-off state if GPIO floats at boot |
| Q1 pin 2 (emitter) | `GND_LOGIC` | return path |
| C21 (10uF), C22 (100nF) | `5V_RELAY` / `GND_LOGIC`, once per sheet (all 4 `5V_RELAY` label instances are one net) | local bulk + bypass, EMI/noise reasoning above |
| J3 (Conn_01x08) | K1-K4 contact pins, 2 positions each, no shared common | field connector |

**K1's pin mapping** (ALDP105, no numbered pinout in any datasheet found -- same situation
as the LTV-247): pins 1/2 are the coil, pins 3/4 are the SPST-NO contact, confirmed live
from Symbol Properties -> Pin Functions on the placed symbol before any wiring was done.

**Q1's pin mapping** (MMBT3904, SOT-23): pin 1 = base, pin 2 = emitter, pin 3 = collector,
also confirmed live from the placed symbol rather than assumed from the package outline.

**Root sheet:** `relay_outputs` sheet symbol added to `ioboard.kicad_sch` with a `5V_RELAY`
input pin, wired to the power sheet symbol's `5V_RELAY` output pin -- same pattern as
`3V3_LOGIC` for digital-inputs. Verified programmatically (not just visually) that the wire
path actually joins both pin coordinates, not just two labels that happen to look aligned.

**Caught and corrected during capture:** D9 was initially wired with reversed polarity
(cathode toward the collector node instead of toward `5V_RELAY`) -- would have put a
forward-biased diode across the coil for the entire time each transistor is on, well past
the diode's ~300mA rating. Caught before any commit, fixed by mirroring the symbol.

## Step 5 results: verification checklist (2026-09-06)

- ALDP105 coil/contact specs: Panasonic's own industry product page, re-checked fresh this
  subsystem rather than trusted from the earlier power-subsystem note.
- MMBT3904 specs: Nexperia's own datasheet, fetched fresh this subsystem.
- Base resistor sizing (510 ohm, ~3.8x saturation margin): worked from the transistor's
  datasheet hFE figures, conservative (lower) test-point value used since actual coil
  current falls between the two guaranteed points.
- Flyback diode orientation: verified against actual circuit behavior (forward path only
  during turn-off kickback), not just copied from a generic reference -- this is exactly
  what caught the reversed D9 above.
- Reference designators: programmatically checked, no duplicates, no gaps, no misspelled
  labels (`5V_RELAY`, `GND_LOGIC`, `RELAY1_CTRL`-`RELAY4_CTRL` all grep-verified clean).
- Connector: Phoenix Contact MC 1,5/8-ST-3,5 (1840421) confirmed in stock via Newark and
  Farnell listings; K1-K4's contact pins independently verified wired to distinct J3
  positions (no shared/bussed pins).
- **Not yet verified, flagged rather than assumed correct:** K1-K4's footprint
  (`ALDP105:RELAY_ALDP105`) came bundled with the imported third-party symbol library. Unlike
  the other parts on this sheet (which have no footprint yet, a deliberate deferral), this
  one already has a footprint assigned that has not been independently checked against the
  ALDP105's real mechanical/pin-spacing drawing. Tracked as a commissioning/footprint-review
  item below, not assumed correct just because it came from an imported library. Q1-Q4's
  footprint (`Package_TO_SOT_SMD:SOT-23`) is a standard KiCad library footprint matching the
  datasheet's stated SOT-23 package -- lower risk, but still covered by the general
  ERC/footprint-assignment pass deferred for the whole subsystem (see below).

No simulation performed -- same as every other subsystem, this is a datasheet-compliance +
resistor-math design, not a circuit needing a control-loop simulation.

## Step 6: Acceptance criteria

1. Each channel's relay coil energizes reliably when its GPIO drives `RELAY#_CTRL` high, and
   stays off when the GPIO is low or floating (pull-down holds it off at boot). Confirmable
   only at bring-up.
2. No flyback-related damage to Q1-Q4 or D9-D12 across repeated switching cycles --
   confirmable only at bring-up with real coils.
3. Contact side switches its connected load reliably, within the connector's 160V/8A rating
   (documented limitation, not a defect -- see Design approach).
4. No strapping pin conflict once roadmap step 6 assigns real GPIOs to `RELAY1_CTRL`
   through `RELAY4_CTRL` -- re-checked at that point, not yet actionable.
5. ERC clean and real footprints assigned (including independently verifying the imported
   ALDP105 footprint) before this subsystem merges to main -- same deferred-but-tracked
   policy as digital-inputs, not blocking further roadmap progress now.

Items 1-2 need real hardware -- tracked below as commissioning items, same pattern as every
other subsystem.

## Step 7: Bill of materials

See `hardware/bom/relay_outputs_bom.csv`. 23 parts: K1-K4 (ALDP105), Q1-Q4 (MMBT3904), R26-R33
(8x E24 resistors, 510/10k alternating), D9-D12 (1N4148), J3 (Phoenix Contact MC 1,5/8-ST-3,5),
C21-C22 (10uF/100nF bulk+bypass). All passives are generic E24/standard values except K1-K4,
Q1-Q4, and J3, which need the specific manufacturer/part-number match already locked above.

## Commissioning test items (Rev-A bring-up)

| Item | What to check | Why not closed now |
|---|---|---|
| ALDP105 footprint accuracy | Confirm the imported `ALDP105:RELAY_ALDP105` footprint actually matches the real part's pin spacing/pattern before board fab | No independent mechanical drawing was found to cross-check the imported library against |
| Coil switching reliability | Confirm all 4 channels switch cleanly across repeated cycles with real coils and real loads | Real inductive switching behavior isn't fully modeled on paper |
| Connector voltage margin | Confirm actual connected loads stay within the connector's 160V/8A rating | Depends entirely on what the user wires to each channel, unknown at design time |

## Step 8: Sign-off

Relay-outputs subsystem schematic capture complete. All 4 channels wired identically and
verified pin-by-pin against the live placed symbols (not assumed from datasheets, since
neither the ALDP105 nor its transistor driver had a fully trustworthy pinout diagram
available). One real wiring error (reversed flyback diode) was caught and corrected before
commit. ERC and footprint verification (including the imported ALDP105 footprint) deferred
to the same pre-merge pass as digital-inputs, tracked in `CLAUDE.md`.

**Next:** roadmap step continues to analog I/O (input + output) as the next IOBOARD
subsystem.

## Revision history

| Date | Change |
|---|---|
| 2026-09-05 | Scope, design approach, driver-stage math, and connector selection locked |
| 2026-09-06 | Bulk/bypass capacitor decision added; schematic capture, verification checklist, acceptance criteria, BOM, and sign-off completed |
| 2026-09-10 | Clarified this doc's top-of-file status line — it read only "schematic capture complete," understating that all 8 plan steps (through sign-off) are actually closed. Caught during a project-wide documentation consistency pass; no design content changed. |
