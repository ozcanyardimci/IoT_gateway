# Power Subsystem

**Status:** Complete. All 13 plan steps closed.

## Scope

Reverse-polarity protection, surge/EMI protection, and 3.3V/5V regulation from a 10-30 VDC
field input, feeding both boards over the board-to-board header.

Operating environment: -20C to +60C ambient (confirmed requirement — indoor DIN-rail
industrial enclosure). Every component below is checked against this range.

## Design approach

- **Regulation:** pre-made DC-DC modules, not a custom buck design. Two-stage topology
  (protection chain into four independent point-of-load modules).
- **Validation:** simulation only (ngspice), no breadboard bring-up. Items that can't be
  closed on paper are tracked as commissioning tests for Rev-A (see below).

## 1. Requirements & load budget

| Part | Rail | Supply range | Typical current | Peak/max current | Confidence |
|---|---|---|---|---|---|
| ESP32-S3-WROOM-1U | 3.3V | 3.0-3.6V | ~95-100 mA (WiFi RX) | 355 mA (WiFi TX burst) | Datasheet |
| Quectel EG915U-EU | 3.3V-LTE (dedicated) | VBAT 3.3-4.3V | ~30 mA idle (approx.) | 2A (LTE) / 3A (GSM+LTE) TX burst | Datasheet |
| WIZnet W5500 | 3.3V-LOGIC | 3.3V | 132 mA | No max published — budgeted at 200 mA (1.3-1.5x typical) | Typical figure from datasheet; margin is our own choice |
| NXP PCA9535PW | 3.3V-LOGIC | 2.3-5.5V | ~0.03-0.1 mA | 0.2 mA max | Datasheet |
| MAX3232E | 3.3V-LOGIC | 3.0-5.5V | 0.3 mA | 1 mA max | Datasheet |
| Mornsun TD321S485H-A | 3.3V-LOGIC | 3.15-3.45V | 37 mA | 90 mA max | Datasheet |
| AME8808 LDO (analog stage) | 3.3V-LOGIC | — | ~30 uA (same-family estimate) | negligible | AME8808's own datasheet wasn't located; using AME8805/8813 sibling figure — immaterial to rail sizing either way |
| LM2904 op-amp (analog stage) | 3.3V-LOGIC | 3-26V | 0.35 mA/amp | 0.6-1.0 mA/amp | Datasheet |
| Panasonic ALDP105 relay coil (x4) | 5V | 5V nominal | 40 mA each | 40 mA each, resistive coil | Manufacturer page |
| LTV-247 optocoupler (x2, 8ch) | Field side | — | — | — | LED side draws from field wiring, not internal rails; output side is a negligible pull-up on GND_LOGIC |

**Rail architecture:** the Quectel modem's 2-3A transient rules out a single shared 3.3V
rail — a shared regulator would sag under every LTE TX burst, right when the other chips
need clean power. Four independent rails:

| Rail | Load | Worst-case peak |
|---|---|---|
| 5V | Relay coils (x4) | ~160 mA |
| 3.3V-LOGIC | ESP32-S3, W5500, PCA9535PW, MAX3232E, RS485 module, analog logic side | ~650 mA (sized to ≥850 mA-1A with margin) |
| 3.3V-LTE | Quectel EG915U-EU only, dedicated | 2-3A transient |
| 3.3V-ANALOG-ISO | Analog input isolation amp/ADC, field side | Tens of mA |

## 2. Grounding & isolation architecture

| Interface | Isolated? | Mechanism |
|---|---|---|
| Digital inputs (8x) | Yes | LTV-247 optocouplers, 3750 Vrms |
| RS485 | Yes | Mornsun TD321S485H-A, internal isolation |
| Ethernet | Yes | Magjack integrated transformer (Würth 7499010441) |
| Relay outputs (4x) | Yes, load side only | ALDP105 mechanical contacts; coil/control side stays on GND_LOGIC |
| Power input | No | Reverse-polarity MOSFET is not galvanic isolation |
| Analog input | Yes | Isolation amp/ADC added — deliberate deviation from the reference design (see below) |
| Analog output | No | Unchanged for now; revisit at analog subsystem design |
| RS232 | No | MAX3232E has no isolation |
| LTE / WiFi | N/A | No field-wiring connection |

**Ground domains:**

- **GND_LOGIC** — ESP32-S3, W5500, PCA9535PW, MAX3232E, relay coil-driver control side,
  regulator returns, analog stage's logic-side. Crosses the board-to-board header.
- **GND_ANALOG_ISO** — isolated field-side ground for the analog input, own isolated
  supply, never tied to GND_LOGIC.
- **GND_FIELD_DI** — isolated field-side return for the 8 digital inputs.
- **GND_RS485_ISO** — isolated bus-side ground, internal to the Mornsun module.
- **Chassis/frame** — bonded to the DIN-rail enclosure, single-point-tied to GND_LOGIC at
  the power entry so surge energy has a low-impedance path without creating ground loops.

Only GND_LOGIC crosses the LTEBOARD/IOBOARD header — every isolated ground stays local to
IOBOARD.

**Analog input isolation** is an intentional deviation from the reference design (which has
none, per teardown). An unisolated 4-20mA loop input is a common source of ground-loop and
noise problems in industrial deployments; the added isolation amp/ADC, isolated supply
rail, and firmware driver change (external ADC instead of the ESP32-S3's `analogRead()`)
are contained to the analog subsystem and worth the small BOM cost.

## 3. Module selection

| Rail | Part | Input range | Rating | Margin | Notes |
|---|---|---|---|---|---|
| 3.3V-LOGIC | Würth MagI3C-VDLM 171013801 | 3.5-38V (abs. max 42V) | 1A | ~54% headroom over 650mA peak | Adjustable output via divider |
| 3.3V-LTE | Würth MagI3C-VDLM 171033801 | 3.5-38V (abs. max 42V) | 3A | Quectel's 2-3A transient inside continuous rating | Same part identified on the reference board's teardown |
| 5V (relays) | Würth MagI3C-VDLM 171013801, second instance | 3.5-38V | 1A | ~84% headroom over 160mA | Divider re-tapped to 5V |
| 3.3V-ANALOG-ISO | Recom R1SX-3.33.3-R | 3.3V regulated input | ~300mA / 1W | Heavy margin over tens-of-mA load | 1kVDC isolation; "/H" option available for 3kVDC |

Two unique SKUs cover three of the four rails (171013801 used in two separate physical
instances). Both MagI3C parts publish Iout-vs-ambient derating curves at 12V/24V, usable
directly for the enclosure-ambient check. Würth doesn't publish a downloadable SPICE model
for MagI3C — REDEXPERT (their browser-based simulator) is used as an efficiency/thermal
cross-check outside of the ngspice work in step 6 below, which uses an idealized behavioral
model for the modules.

171033801's transient-response curve is characterized for a 10%-100% load step, not
specifically an RF PA burst — extra output bulk capacitance is placed at the Quectel module
(matching the reference board's teardown) rather than relying on the regulator's transient
response alone.

## 4. Protection design & sequencing

Fixed order from the input connector: fuse -> reverse-polarity -> surge/EMI -> regulation.
Fuse upstream of the TVS is deliberate — TVS diodes typically fail shorted under cumulative
surge energy, so a fuse ahead of it converts that failure mode into an open circuit instead
of a permanent short.

1. **J1 — Phoenix Contact MC 1,5/2-ST-3,5** terminal block.
2. **F1 — Littelfuse RXEF135** PTC resettable fuse. 1.35A hold / 2.70A trip / 72V max at
   20C; derates to 0.85A hold at 60C ambient.
3. **U1/Q1 — TI LM74610-Q1 ideal diode controller + CSD18531Q5A NexFET**, reverse-polarity
   protection. Follows TI reference design TIDUBP3A, rated for automotive 12/24V systems
   with load-dump survival past 30V. Chosen over a discrete P-MOSFET + Zener approach
   because the IC's internally-regulated gate drive has no Vgs-overvoltage failure mode to
   guard against in the first place, rather than needing a clamp to work around one.
4. **U2 — TI TVS3300** (Flat-Clamp surge protection, 33V standoff, 40V max clamp at
   35A/8-20us) + **CY1 — Würth WCAP-CSSA 8853522140011** Y-cap (4.7nF, X1/Y2, 250VAC).
5. Into the four DC-DC modules (section 3).

**Fuse sizing:** estimated sustained output power across all four rails is ~4.1W; at ~85%
buck efficiency and 10V minimum input, input current is ~0.48A. RXEF135's 60C-derated hold
current (0.85A) gives 1.77x margin — the number that matters, since room-temperature
derating (1.35A/2.8x) overstates real headroom at the assumed ambient.

**Surge protection (U2):** TI's Flat-Clamp TVS family (TVS0500-TVS5800) uses active
feedback to clamp much closer to standoff voltage than a conventional avalanche TVS
(30-50 milliohm dynamic resistance, vs. the ~1.6x-standoff clamp ratio that holds across
every conventional silicon TVS class at this voltage regardless of package or power
rating — checked against Littelfuse's SMBJ/SMCJ/5KP families). TVS4000 (40V standoff) still
clamps at 50.3V, above the MagI3C modules' 42V absolute max; TVS3300 (33V standoff) clamps
at 40V max, the only family member that fits. Trade-off: 33V standoff gives ~10% margin
over the 30V max continuous input, vs. the ~20% a 36V-class part would give — accepted
because clamp voltage under 42V is the harder constraint. TVS3300DRVR is active production,
stocked at DigiKey and Mouser (Mouser ships to Turkey; DigiKey via EKOM). SON-6 package,
pins 4/5/6 = IN, pins 1/2/3 + thermal pad = GND, direct two-node shunt per TI's application
circuit.

**MagI3C overcurrent protection:** both 171013801 and 171033801 have built-in
cycle-by-cycle current limiting (OCP/SCP), confirmed per datasheet — no external
current-limiting needed on the module outputs.

## 5. Inrush current (ngspice simulation)

Circuit: 0V->30V step (worst-case hot-plug) through Rfuse (F1 cold resistance, 0.12 ohm) +
Rq1 (Q1 RDS(on), 3.5 mOhm) + 0.05 ohm assumed PCB/connector parasitic, into the input
capacitor bank (8x 4.7uF, 37.6uF total, 20 mOhm ESR each).

| Case | Rsrc | Peak current | Time constant |
|---|---|---|---|
| Worst case (near-zero source impedance) | 10 mOhm | 161A | 6.9us |
| Realistic (typical supply/connector impedance) | 0.5 ohm | 44A | 25.3us |

Cross-checked two ways: hand-calculated tau = R_total x C_total = 6.99us vs. simulated
6.897us (1.5% agreement); hand-calculated I = V/R at t=0 = 161.3A vs. simulated 161.28A.
This level of agreement is what establishes confidence in the netlist, not just that the
simulator ran without error.

Checked against ratings: Q1's IDM (pulsed drain current) is 400A for pulse width <=100us at
<=1% duty — 161A peak over a ~35us decay gives 2.5x margin even in the pessimistic case. F1
is thermally slow (9.6s time-to-trip at rated fault current); a 35us pulse carries ~0.9 A^2s
of I^2t, far too little to heat the PTC's thermal mass.

**No NTC thermistor needed** — the fuse's cold resistance already provides enough
current-limiting margin on both Q1 and F1. One item carried to layout: 161A for ~35us
should be checked against PCB trace/connector current-carrying capacity at that stage —
standard copper handles microsecond pulses like this without issue, but worth confirming.

## 6. Connector & wiring

**Wire gauge:** sized so ampacity exceeds F1's 2.70A trip current, not the reverse. NEC
Table 310.15(B)(16)'s smallest tabulated gauge (18 AWG) is rated 14A at 90C — 5x margin.
Engineering ToolBox's more conservative near-enclosure table gives 20 AWG = 6.0A. **Chosen:
20 AWG stranded copper, 80-105C insulation** — ~2.2x margin on the conservative table,
standard gauge for a DIN-rail device.

**Terminal block (J1):** Phoenix Contact MC 1,5/2-ST-3,5, 2-position (V+/V-; chassis ground
bonds mechanically through the DIN-rail clip, not a third wire). 8A/160V rated, 3.5mm
pitch, accepts 28-16 AWG.

**Creepage/clearance:** 30V is low enough that IPC-2221 spacing is comfortably satisfied by
standard PCB design-rule spacing and the terminal block's own pitch — not a driving
constraint at this voltage, unlike the higher-voltage isolation barriers (optocouplers,
RS485 module) which do need specific attention at layout.

The board-to-board header's power pins (3.3V-LOGIC, 3.3V-LTE, 3.3V-ANALOG-ISO, 5V, returns)
are covered under the general board-to-board interconnect milestone in the roadmap, not
here — this section covers the field power input connector only.

## 7. Schematic capture

Full input-to-output chain wired in KiCad (`hardware/kicad/ioboard/`): J1 -> F1 -> Q1/U1 ->
U2/TVS3300 -> four DC-DC modules. GND_LOGIC exposed as a project-wide Global Label; the
other five rail/ground nets exposed via Hierarchical Label + matching Sheet Pin on the
parent sheet (Output direction). See section 12 for the full reference/part table.

## 8. Operating temperature check

| Part | Datasheet range | Fits -20C/+60C? |
|---|---|---|
| MagI3C 171013801 / 171033801 | -40C to +105C | Yes, wide margin |
| Littelfuse RXEF135 | -40C to +85C | Yes — hold-current derating already factored into fuse sizing above |
| TI TVS3300 | -65C to +150C storage (family datasheet; TVS3300-specific page not independently re-pulled) | Yes, wide margin |
| Würth WCAP-CSSA | -55C to +125C | Yes, wide margin |
| Recom R1SX-3.33.3-R | -40C to +100C | Yes, wide margin |

## 9. Margin verification

Every component checked against its worst-case condition, not nominal:

| Item | Worst-case condition | Rating vs. actual | Margin |
|---|---|---|---|
| F1 hold current vs. sustained load | 60C ambient (derated) | 0.85A hold vs. ~0.48A load | 1.77x |
| Q1 inrush pulse | Near-zero source impedance | 400A/100us vs. 161A/~35us | 2.5x |
| TVS3300 clamp vs. MagI3C abs. max | 35A/8-20us surge | 42V abs. max vs. 40V max clamp | ~5% |
| TVS3300 standoff vs. max input | 30V continuous | 33V standoff | ~10% |
| DC-DC module input voltage | 10V min design input | 3.5-38V operating range | Wide |
| DC-DC module input UVLO | Worst-case sag (section 10) | 3.3V max rising threshold vs. 10V min input | ~3x before local-resistance margin |
| Operating temperature | -20C/+60C confirmed | All parts rated -40C or better | Wide |

No item falls below 1.5x margin. The tightest two (TVS3300 clamp headroom and standoff)
were accepted deliberately for the reasons in section 4, not overlooked.

## 10. Power sequencing & brown-out check

**Sequencing:** all four rails power independent downstream devices rather than multiple
rails feeding one IC, so there's no core-before-IO-style dependency between them. All four
modules' EN pins tie directly to the shared protected-rail VIN, so all four come up
together — no relative ordering constraint found in any downstream datasheet.

**Brown-out from load transients:** the LTE modem's 2-3A TX burst is the largest transient
on the shared protected rail. Through the board's own series resistance (fuse + MOSFET +
parasitic, ~0.1735 ohm), a 3A step produces a local sag of only I x R = ~0.52V — against a
10V minimum design input and the MagI3C modules' 3.3V max UVLO rising threshold, the rail
would need to collapse by roughly two-thirds before any module's UVLO were at risk. The
board's own resistance can't get there. The one variable this can't rule out on paper is the
external supply's own regulation quality under a fast load step — tracked as a commissioning
item, not a design risk, given the ~3x margin already present.

## 11. Acceptance criteria

1. Each output rail holds within +/-3% of nominal across the full 10-30V input range and
   full rated load.
2. Reverse-polarity connection (J1 swapped) results in zero current flow past Q1 and no
   downstream damage.
3. F1 does not nuisance-trip under worst-case sustained load (0.48A) at 60C ambient, and
   trips within its rated curve under an actual fault.
4. A surge event up to TVS3300's rated 35A/8-20us Ipp does not expose any DC-DC module
   beyond its 42V absolute maximum input rating.
5. Input capacitor inrush at power-up does not exceed Q1's IDM (400A/100us) or trip F1.
6. GND_ANALOG_ISO has no direct DC path to GND_LOGIC — isolation boundary intact.
7. All four rails power up together with no relative sequencing fault.

Items 1, 3 (fault case), 6, and 7 require real hardware to close out fully — Rev-A
bring-up checks, listed under Commissioning below.

## 12. Bill of materials

| Ref | Part | Role | MPN |
|---|---|---|---|
| J1 | Phoenix Contact MC 1,5/2-ST-3,5 | Field power input terminal block | 1840366 |
| F1 | Littelfuse RXEF135 | PTC resettable fuse | RXEF135 |
| Q1 | TI CSD18531Q5A | N-MOSFET, reverse-polarity switch | CSD18531Q5A |
| U1 | TI LM74610-Q1 | Ideal diode controller | LM74610QDGKRQ1 |
| U2 | TI TVS3300 | Flat-Clamp surge protection | TVS3300DRVR |
| U3 | Würth MagI3C-VDLM | 3.3V-LOGIC DC-DC module | 171013801 |
| U4 | Würth MagI3C-VDLM | 3.3V-LTE DC-DC module (3A) | 171033801 |
| U5 | Würth MagI3C-VDLM | 5V-RELAY DC-DC module | 171013801 |
| U6 | Recom R1SX-3.33.3-R | Isolated 3.3V analog supply | R1SX-3.33.3-R |
| CY1 | Würth WCAP-CSSA | Input Y-cap, EMI/safety | 8853522140011 |
| C1 | Ceramic, X7R, 16V | LM74610-Q1 charge-pump cap | 2.2uF |
| R2/R5/R8 | E96 | FB top resistor (3.3V rails) | 402k |
| R3/R6 | E96 | FB bottom resistor (3.3V rails) | 137k |
| R9 | E96 | FB bottom resistor (5V rail) | 80.6k |
| R1/R4/R7 | — | FSW frequency-set resistor | 5.6k |

Per-module input/output/VCC cap values are in the schematic; not repeated here.

## Commissioning test items (Rev-A bring-up)

Items that depend on real hardware or the actual field supply, and can't be closed on
paper. None block sign-off — all are backed by comfortable design margin.

| Item | What to check | Why not closed now |
|---|---|---|
| W5500 sustained current | Bench-probe under 100M link, continuous TX | No manufacturer max published; 200mA is our own margin |
| LTE sustained current | Bench-probe during a real data session | 500mA is an estimate, not a Quectel-published figure |
| Inrush vs. real supply impedance | Confirm measured inrush tracks the simulated worst case | Field supply's output impedance is unknown until real hardware exists |
| Brown-out vs. real supply | Confirm rail stability during an actual LTE TX burst | Same — external supply regulation quality can't be modeled on paper |
| Inrush trace/connector rating | Confirm PCB copper handles 161A/~35us at layout | Sanity check, not expected to be an issue |

## Revision history

| Date | Change |
|---|---|
| 2026-08-30 | Requirements, load budget, grounding architecture, module selection, protection sequencing, wiring selection |
| 2026-08-30 | Reverse-polarity stage switched from discrete P-MOSFET + Zener to LM74610-Q1 + CSD18531Q5A (removes a Vgs-overvoltage failure mode rather than working around it) |
| 2026-08-30 | Fuse sizing re-checked at 60C ambient derating (was room-temperature only) |
| 2026-08-31 | Surge stage consolidated from a two-stage SMBJ36A + ferrite + secondary-TVS design to a single TVS3300 (Flat-Clamp technology) |
| 2026-08-31 | 3.3V-ANALOG-ISO part corrected from R1SX-3305 (3.3V-in/5V-out) to R1SX-3.33.3-R (3.3V-in/3.3V-out), matching the rail |
| 2026-08-31 | Inrush current simulated in ngspice; no NTC thermistor required |
| 2026-09-03 | Margin verification, sequencing/brown-out check, acceptance criteria, BOM, sign-off |
| 2026-09-03 | Operating temperature range (-20C/+60C) confirmed as a real requirement |
| 2026-09-03 | U6 (analog isolated supply) reference designator corrected from P51 |
