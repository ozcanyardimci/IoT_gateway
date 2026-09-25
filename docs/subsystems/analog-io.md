# Analog I/O Subsystem — Build Plan

**Status:** Complete. All 9 plan steps closed 2026-09-07. Schematic capture verified
pin-by-pin against the saved `analog_io.kicad_sch` file, and ERC run to a fully-explained
clean state (0 unexplained violations; 15 documented exclusions), not by self-report.

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
the already-sized 5V_RELAY rail without redefining it. **Done 2026-09-08** — see the
superseded-part note under Step 1 below for the one thing that changed along the way (final
part differs from this doc's own candidate).

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
| Candidate part (superseded, see below) | Recom R05P215S |
| Input | 5V (from the existing, already-budgeted 5V-RELAY rail — not a new raw-field tap) |
| Output | 15V |
| Rated power/current | 2W / 133mA |
| Isolation | Same Econoline family class as the already-used R1SX-3.33.3-R; exact kVDC
  figure for this specific part not independently re-verified this session — confirm
  against the datasheet PDF directly before BOM lock, same "not yet re-verified" flag used
  elsewhere in `hardware/datasheets/README.md` |

**Superseded 2026-09-08 (kept for traceability):** when this rail was actually added to
`power.md`, the part landed on was **Recom RK-0515S** (2W, 66mA, 3kVDC isolation stated
directly — no re-verification caveat needed), not R05P215S above. Same role (5V-RELAY in,
15V out, feeds the output gain stage), different specific part — R05P215S's unresolved
isolation-rating gap was the reason to look further rather than lock it in. See `power.md`
section 3 for the part actually implemented, and its own revision history for when.

**Load check:** this rail only feeds the output gain stage's LM2904 half — quiescent draw
~0.35mA, plus output load current (a downstream analog input is typically >=100k ohm
impedance, so <=100 uA at 9.91V). Total draw is comfortably under 1mA against RK-0515S's
66mA rating (was 133mA against the superseded candidate above) — heavy margin either way,
consistent with this project's "proven module with margin, not a custom design sized to the
bare load" pattern for every other rail.

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
- ~~New 15V rail's addition to `power.md`~~ **Done 2026-09-08** — added as `15V-ANALOG-ISO`
  (Recom RK-0515S, not the R05P215S candidate above — see the superseded note under Step 1).
  The `15V_ANALOG_ISO` root-sheet pin on the `analog_io` sheet symbol was left dangling until
  then; wiring it up on `ioboard.kicad_sch` is a small remaining hookup, not a re-design —
  not yet confirmed done, check against the current `ioboard.kicad_sch` before this
  subsystem's next touch.

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
- **R_in1 (49.9k) / R_f1 (100k)** — output-stage feedback network. Verified the "-" pin,
  R_in1's GND-side leg, and R_f1's output-side leg all meet at one shared node (the actual
  feedback-loop requirement for a non-inverting stage) — an earlier wiring pass had this
  wrong (R_f tied to GND instead of to the "-"/R_in node, which breaks feedback entirely)
  and was caught and corrected during capture. Renamed from bare `R_in`/`R_f` to `R_in1`/
  `R_f1` by KiCad's own Annotate tool during the Step 6 ERC pass (bare reference designators
  without a numeric suffix are not valid final references) — value and topology unchanged.
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

## Step 6 results: verification checklist (2026-09-07, done)

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

**KiCad ERC (Electrical Rules Check) — run to completion, on `analog_io.kicad_sch` plus the
directly-tied root-sheet pin.** Final state: 0 unexplained violations; 15 findings reviewed
and excluded with a written, per-violation comment (stored in `ioboard.kicad_pro`'s
`erc_exclusions`, not silently suppressed). Final `ERC.rpt` saved alongside the project files.

Two real, cross-sheet issues were caught by this ERC pass and fixed directly (not excluded —
these were genuine bugs, in files outside this sheet):

- **Duplicate reference collision:** `power.kicad_sch`'s reverse-polarity-protection FET was
  also `Q1` — colliding with `relay_outputs.kicad_sch`'s own Q1-Q4 relay driver transistors.
  Renamed to `Q5` in `power.kicad_sch`. Documented here because it was this subsystem's ERC
  run that surfaced it, even though the fix lands in a different sheet; see `power.md`'s own
  revision history for the authoritative record of that change.
- **Inconsistent GND_LOGIC labeling:** `power.kicad_sch` had one `global_label "GND_LOGIC"`
  plus four separate plain `label "GND_LOGIC"` instances scattered across the sheet — a
  `same_local_global_label` warning. All four converted to global labels (KiCad's local
  labels only tie together same-named instances *within one sheet*; this net needs to be
  global project-wide). Same cross-sheet note as above — recorded authoritatively in
  `power.md`.
- Bare `R_in`/`R_f` reference designators renamed to `R_in1`/`R_f1` by KiCad's Annotate tool
  (see Step 5 above) — not a bug, but worth noting as part of the same ERC pass.

The 15 exclusions, grouped by root cause:

1. **Cross-sheet power-driver false positives (4):** U9 VDD, U9 GND, U10 VCC2 — all
   `power_pin_not_driven`. KiCad's ERC does not trace power drivers across sheet-hierarchy
   boundaries; each of these rails genuinely is driven, by power.kicad_sch's isolated DC-DC
   module (`U6 +VOUT` / `-VOUT`) via a hierarchical sheet pin. Confirmed by checking that a
   real `Output`-type pin exists upstream before excluding (see "mistake avoided" below).
2. **Documented spare/deferred headroom (7):** U9 AIN2 + AIN3 (`pin_not_connected` and
   `pin_not_driven` each — spare ADC channels, 2 of 4 used, per Step 1); U9 ALERT/RDY
   (`pin_not_connected` — spare output pin, not used by this polling-based design); U13 unit
   B (`missing_input_pin` and `missing_unit` — the spare LM2904 half flagged as headroom back
   in Step 1's "Known headroom" note).
3. **Deferred GPIO/MCU-pin assignment (2):** `SDA_ISO` and `SCL_ISO` local labels
   (`label_dangling`) — same deferral pattern as relay-outputs' `RELAY#_CTRL` labels, per
   roadmap step 6.
4. **Deferred 15V_ANALOG_ISO rail (3):** the analog_io-sheet hierarchical label
   (`label_dangling`), U13 Pin 8 V+ (`power_pin_not_driven`), and the root-sheet hierarchical
   sheet pin (`pin_not_connected`) — all three tied to the same not-yet-added rail (see
   "Power subsystem impact" above; this is `power.md`'s job, on `main`, after this subsystem
   closes).

**A mistake made and reversed during this pass, worth recording:** the first attempt at
fixing the 4 cross-sheet power-driver false positives used `PWR_FLAG` symbols instead of ERC
exclusions. That was wrong — `PWR_FLAG` tells ERC "trust me, nothing drives this net," which
is only correct when a net truly has no driver anywhere in the design. These nets *do* have
real drivers (power.kicad_sch's regulator outputs); adding `PWR_FLAG` anyway created a new,
genuine conflict (`"Pins of type Output and Power output are connected"` against those same
regulator pins). Caught by re-running ERC before treating the fix as final, reverted, and
redone correctly as documented exclusions instead.

Deliberately left **un-excluded** (out of scope for this subsystem — pre-existing, belonging
to other, already-signed-off subsystems, or project-wide policy):

- `VBAT_LTE` (renamed from `3V3_LTE` 2026-09-13, see `power.md`) hierarchical sheet pin
  unconnected — a power/LTE board-boundary question, not this subsystem's; will resolve
  once `lte.kicad_sch` is wired.
- `RELAY1_CTRL`...`RELAY4_CTRL` dangling labels — relay-outputs' own deferred GPIO
  assignment, same roadmap-step-6 pattern, but that subsystem's item to close, not this one's.
- 9 `footprint_link_issues` warnings — the project-wide "footprints not yet verified before
  PCB layout" policy already documented in `hardware/datasheets/README.md` and
  `hardware/kicad/README.md`; not specific to analog-io.

Physical checks remain deferred to Rev-A hardware bring-up (unchanged from the original plan
below): rail presence and isolation (megohmmeter check between GND_ANALOG_ISO and GND_LOGIC),
I2C bus scan confirming both devices ACK at their expected addresses, AI1/AI2 functional test
against a known input voltage, AO functional test against a commanded DAC code. None of this
is possible before PCB fabrication; Step 7 below formalizes pass/fail thresholds for each.

## Step 7 results: acceptance criteria (2026-09-07)

Schematic-level acceptance (all met, this doc):

- ERC clean to a fully-explained state (Step 6) — met.
- No accidental GND_ANALOG_ISO / GND_LOGIC bridging outside the ISO1540 barrier — met.
- No I2C address collision between ADS1115 and MCP4725 — met.
- Output gain-stage feedback topology correct (non-inverting, closed loop through R_in1/R_f1)
  — met.
- Every rail this sheet needs is either already routed from `power.kicad_sch`
  (3V3_ANALOG_ISO, GND_ANALOG_ISO, 3V3_LOGIC) or explicitly tracked as a deferred, named gap
  with an owner (15V_ANALOG_ISO -> `power.md`, on `main`) — met, nothing silently missing.

Physical acceptance (Rev-A hardware bring-up, not yet possible — no PCB exists):

- Isolation barrier holds: megohmmeter reading between GND_ANALOG_ISO and GND_LOGIC planes
  exceeds manufacturer isolation spec (ISO1540: 2500 Vrms continuous) with no continuity.
- I2C bus scan finds both devices at their designed addresses (0x48, 0x60) and no others.
- AI1/AI2: applying a known 0-10V or 4-20mA reference signal (per populated stuffing option)
  reads back within the ADC's specified accuracy at both ends of the input range.
- AO: commanding a known DAC code produces the corresponding voltage at J4 pin 3, within the
  gain stage's component-tolerance budget, across the 0-9.91V design range.
- 15V_ANALOG_ISO rail (once `power.md` adds it): present, within regulation, and the LM2904
  output stage does not clip across the full commanded output range.

These thresholds are the pass/fail bar for Rev-A bring-up; they are not being claimed as met
here, since no physical board exists yet.

## Step 8 results: documentation & BOM (2026-09-07)

- **This doc** (`docs/subsystems/analog-io.md`) — kept current through every step, including
  this one; see revision history below.
- **BOM:** `hardware/bom/analog_io_bom.csv` created (see repo) — every part from Step 1
  (U9 ADS1115IDGSR, U10 ISO1540, U11 MCP4725A0T-E/CH, U12/U13 LM2904 x2, D13/D14 SMBJ15CA,
  R_top1/R_bottom1/R_top2/R_bottom2/R_in1/R_f1) plus Step 3's connector (J4, Phoenix Contact
  MC 1,5/4-ST-3,5), matching the reference designators actually in `analog_io.kicad_sch` —
  cross-checked designator-by-designator against the saved schematic file, not copied from
  this doc's prose. The 15V_ANALOG_ISO supply module is **not** in this BOM — it's a
  `power.kicad_sch` part and belongs in `power_bom.csv` (added 2026-09-08 as Recom RK-0515S,
  not the R05P215S this doc had in mind at the time — see Step 1's superseded-part note),
  same ownership split as the schematic itself.
- **Datasheets:** part numbers above already appear in `hardware/datasheets/README.md`'s
  tracking table (added/confirmed as part of the project-wide documentation pass done
  alongside this step — see that file's own revision history). Footprint verification stays
  flagged as deferred-to-pre-layout there, consistent with every other subsystem.

## Step 9 results: sign-off (2026-09-07)

All 9 steps of this subsystem's build plan are complete. Next subsystem per the roadmap:
RS485 (now also complete — see `docs/subsystems/rs485.md`). The 15V_ANALOG_ISO rail addition
to `power.md`, flagged here as a carried-forward item, was completed 2026-09-08 (as Recom
RK-0515S, not this doc's own R05P215S candidate — see Step 1's superseded-part note). Not
independently confirmed here: whether the `15V_ANALOG_ISO` root-sheet pin on `ioboard.kicad_sch`
has actually been wired to the new rail, or is still the dangling pin this doc originally
left it as — check before treating that connection as done.

## Steps

1. **Front-end + isolation-crossing design** — real part selection, front-end math,
   isolation-crossing decision — **DONE (this doc)**.
2. **New rail spec** — written into `power.md`, not duplicated here — **DONE (2026-09-08,
   in `power.md`)**, final part (Recom RK-0515S) differs from this doc's own candidate, see
   Step 1's superseded-part note.
3. **Connector selection** — **DONE (this doc)**.
4. **Strapping/reserved pin cross-check** — deferred to roadmap step 6, same as every other
   subsystem.
5. **Schematic capture (KiCad)** — new sheet, `ioboard/analog_io.kicad_sch` — **DONE (this
   doc)**.
6. **Verification checklist** — **DONE (this doc)** — ERC run to a fully-explained clean
   state, 15 documented exclusions.
7. **Acceptance criteria** — **DONE (this doc)**.
8. **Documentation & BOM** — **DONE (this doc)** — `hardware/bom/analog_io_bom.csv` created.
9. **Sign-off** — **DONE (this doc)** — move to the next IOBOARD subsystem (RS485).

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
| 2026-09-07 | Step 6 (ERC) completed: ran KiCad ERC to a fully-explained clean state (15
  documented exclusions, each with a written reason, stored in `ioboard.kicad_pro`). Two real
  cross-sheet bugs caught and fixed along the way: a duplicate `Q1` reference collision
  between `power.kicad_sch` and `relay_outputs.kicad_sch` (renamed to `Q5`), and an
  inconsistent local/global `GND_LOGIC` labeling in `power.kicad_sch` (4 local instances
  converted to global) — both recorded authoritatively in `power.md`. Bare `R_in`/`R_f`
  renamed to `R_in1`/`R_f1` by KiCad's Annotate tool. A `PWR_FLAG` misapplication was caught
  via ERC re-run and reverted before being treated as final. Steps 7 (acceptance criteria), 8
  (documentation & BOM, including new `hardware/bom/analog_io_bom.csv`), and 9 (sign-off)
  completed same day — all 9 steps of this subsystem's build plan are now done. |
| 2026-09-23 | **Final project-wide ERC pass on the merged project.** U9 (ADS1115)'s AIN2, AIN3, and ALERT/RDY pins showed as undriven inputs — all three are genuinely unused by design (this subsystem only needs 2 single-ended analog inputs, ALERT/RDY's comparator-alarm function isn't used), confirmed against TI's own datasheet as safe to leave floating, not a wiring gap. Flagged with No-Connect markers rather than wired. U10 (ISO1540)'s VCC2 "not driven" finding traced to `power.kicad_sch` missing a `PWR_FLAG` on `3V3_LOGIC` (now added, see `power.md`'s matching entry) — nothing to fix on this sheet itself. This subsystem's `I2C_SCL`/`I2C_SDA` hierarchical labels (added at roadmap step 6, see `CLAUDE.md`) now carry the DAC/ADC over the shared I2C bus to `core-compute.kicad_sch`, alongside `status-indication`'s expander — full bus assignment in `docs/architecture.md`. |
| 2026-09-10 | **Correction, project-wide documentation pass.** This doc still described the
  15V rail as an open item with candidate part Recom R05P215S and said the `power.md`
  addition was "written up separately once this doc is locked" — but that addition actually
  happened 2026-09-08, using a different part (Recom RK-0515S). Updated Step 1's table,
  "Power subsystem impact," "Still open," the Steps list, and Step 9 sign-off to reflect
  this. No schematic content changed; this was a documentation lag, not a design change. |
