# Power subsystem — build plan

Status: **not started** (roadmap step 4a). Simulation only, using pre-made DC-DC modules
(no custom-designed regulator, no breadboard bring-up).

## Scope
Reverse-polarity protection, overcurrent/surge protection, and 3.3V/5V regulation from a
10-30VDC field input, feeding both boards over the board-to-board header.

**Operating temperature range (added late, 2026-08-30 — this was missing from the original
step 1 requirements and should have been defined from the start):** assumed **-20C to +60C
ambient**, a common spec for indoor DIN-rail industrial equipment, used to check all step 4-6
component choices below. This was never explicitly confirmed as a real requirement — it's an
assumption, not a decision Ozcan made. If the intended deployment is different (outdoor,
uncontrolled cabinet, etc.), this range and every derating check below needs revisiting.

## Decisions
- **Regulation approach: pre-made DC-DC modules**, not a custom-designed buck converter.
  Faster and lower-risk; trades off some of the "design your own regulator" learning for
  reliability. Two-stage topology expected (pre-regulation step-down, then fixed-output
  point-of-load modules), pending confirmation in step 4.
- **Validation approach: simulation only (ngspice)**, no physical breadboard test. This
  limits what can actually be verified before Rev-A — see the note under step 8.

## Steps

1. **Requirements & specs** — lock the input range (10-30VDC) and required output rails
   (3.3V, 5V). Pull voltage/current requirements for every downstream device from its own
   datasheet.
2. **Load budget** — sum typical and peak current draw per rail, from real datasheet
   numbers rather than estimates. This sizes everything downstream.
3. **Grounding & isolation architecture** — decide whether logic ground, field-power
   ground, and chassis/earth are separate or joined, and where. Settled before parts are
   chosen, since it affects module selection.
4. **Module selection** — choose DC-DC modules whose input range fully covers 10-30V and
   whose current rating carries 20-30% margin over the load budget. Check each candidate's
   thermal derating curve (enclosure ambient, not open-air) and whether the vendor
   publishes a SPICE/behavioral model.
5. **Protection design & sequencing** — fixed order from the input connector: fuse/PTC,
   then reverse-polarity (P-MOSFET), then surge/EMI (TVS + Y-cap), then regulation. Fuse
   sized to the load budget. Surge/EMI margin is designed toward common industrial
   immunity practice (e.g. IEC 61000-4-5-class thinking), without formal certification.
6. **Connector & wiring selection** — terminal block and wire gauge sized to the real
   load, with adequate creepage/clearance for 30V.
7. **Schematic capture (KiCad)** — full input-to-output chain, with test points at every
   stage boundary for later debugging.
8. **ngspice simulation** — validates the protection circuit and loading behavior across
   the full input/load range. Note: since regulation uses off-the-shelf modules, this
   cannot verify the modules' internal control loops — only vendor data and margin
   (step 4) cover that.
9. **Margin verification** — every component's rating checked against worst-case
   conditions, not nominal.
10. **Power sequencing & brown-out check** — confirm whether any downstream device needs a
    specific rail power-up order, and define behavior if input sags briefly (e.g. during
    relay switching) — fail safely, not unpredictably.
11. **Acceptance criteria** — concrete pass/fail numbers, defined before the subsystem is
    marked done.
12. **Documentation & BOM** — final part numbers recorded.
13. **Sign-off** — move to the next subsystem (core compute).

## Known open items carried in from teardown
- No fuse/PTC was identified on the reference board's photos — being added regardless as
  standard practice for a field-powered device (see step 5).
- Relay coil-driver stage between MCU GPIO and relay coil is still unresolved — tracked
  separately under the relay-outputs subsystem, not this one.

---

## Step 1-2 results: requirements & load budget (2026-08-30)

Verified against manufacturer datasheets (not resold/repeated figures). Sources noted per
part; a few gaps are flagged explicitly rather than guessed.

| Part | Rail | Supply range | Typical current | Peak/max current | Source confidence |
|---|---|---|---|---|---|
| ESP32-S3-WROOM-1U | 3.3V | 3.0-3.6V | ~95-100 mA (WiFi RX) | **355 mA** (WiFi TX burst, 802.11b) | High — Espressif official datasheet, cross-verified across 2 copies |
| Quectel EG915U-EU | needs its own rail (see below) | VBAT 3.3-4.3V, never below 3.3V | ~30 mA idle (approximate, table extraction was incomplete) | Supply must be able to **source up to 2A (LTE-only) / 3A (GSM+LTE)** during TX burst | High for the voltage range and 2A/3A supply-capability spec (directly quoted from Quectel hardware design guide); idle/typical mode currents are approximate only |
| WIZnet W5500 | 3.3V | 3.3V (no min/max range published) | 132 mA | **No max current published by WIZnet** — budgeting at 1.3-1.5x typical (~200 mA) since no manufacturer worst-case figure exists | Medium — typical figure is solid, margin above it is our own choice, not a datasheet number |
| NXP PCA9535PW | 3.3V | 2.3-5.5V | ~0.03-0.1 mA | 0.2 mA max | High — NXP datasheet |
| MAX3232E | 3.3V | 3.0-5.5V | 0.3 mA | 1 mA max | High — Analog Devices/Maxim datasheet |
| Mornsun TD321S485H-A (RS485) | 3.3V | 3.15-3.45V | 37 mA | 90 mA max | High — exact part match, Mornsun datasheet |
| AME8808 LDO (analog stage) | 3.3V | not directly verified | ~30 µA (placeholder, from same-family sibling AME8805/8813 — AME8808's own datasheet wasn't found in an extractable form) | negligible either way | Low — flagged, needs the real AME8808 sheet before this is treated as fact, though the current draw is small enough not to matter for rail sizing |
| LM2904 op-amp (analog stage) | 3.3V | 3-26V (single supply) | 0.35 mA/amp | 0.6-1.0 mA/amp max | High — Diodes Inc. datasheet |
| Panasonic ALDP105 relay coil | 5V | 5V nominal | 40 mA each | 40 mA each (resistive coil, no inrush beyond nominal) | High — Panasonic's own page; one reseller listing disagreed (145Ω vs Panasonic's 125Ω) and was discarded in favor of the manufacturer's own figure |
| LTV-247 optocoupler (x2, 8 channels) | **not on our internal rails** | — | — | — | The LED side is driven by the external field digital-input signal through its own series resistor — it draws from the field wiring, not our 3.3V/5V supply. Only the phototransistor output side touches our logic rail, as a passive pull-up — negligible current. |

### Rail architecture conclusion (feeds into step 3 and step 4)
The Quectel modem's 2-3A transient requirement changes the plan from a simple "one 3.3V rail"
design. Sharing a single 3.3V regulator between the modem and the rest of the logic (ESP32-S3,
W5500, PCA9535PW, MAX3232E, RS485 module) risks that regulator's output sagging during every
LTE TX burst, right when the other chips need clean power most. **Decision: three effective
rails**, not two:

- **5V** — relay coils only. Light load (~160 mA worst case, all 4 relays energized).
- **3.3V-LOGIC** — ESP32-S3, W5500, PCA9535PW, MAX3232E, RS485 module, analog LDO/op-amp
  stage. Worst-case simultaneous peak ≈ 355 + 200 + 0.2 + 1 + 90 + ~1 ≈ **~650 mA**; with
  20-30% margin, size this rail's module for **≥ 850 mA-1A**.
- **3.3V-LTE** — dedicated regulator output for the Quectel modem only, sized for **2-3A
  transient capability** with large local bulk capacitance close to the module (matches what
  we saw on the real board's teardown — bulk caps placed right at the modem). Same nominal
  voltage as 3.3V-LOGIC, but electrically a separate output so the LTE transient doesn't
  couple onto the other chips through shared regulator output impedance.

This means step 4 (module selection) is now sizing 3 outputs, not 2, and step 3 (grounding
architecture) needs to account for the LTE rail's return path separately as well.

**Superseded by step 3 below: this is now 4 rails, not 3** — adding isolation to the analog
input (a deliberate improvement over the reference board, decided in step 3) requires its own
small isolated supply. See the step 3 section for the finalized rail count.

### Open items carried forward
- Quectel EG915U-EU per-mode current table (idle/talk/data) wasn't fully extracted from the
  hardware design guide — the 2A/3A supply-capability spec is solid and is what's used for
  sizing, but the idle-current figure above is approximate.
- AME8808's own datasheet wasn't found in extractable form; current draw used is a same-family
  placeholder, not a verified AME8808 number. Low risk given how small the value is either way,
  but should be corrected if the exact datasheet turns up later.
- W5500 has no manufacturer-published max current; the 200 mA figure is our own margin choice.

### Resolution of unverified items (2026-08-30)
Materiality check on the three flagged gaps above:
- **AME8808 exact datasheet** — not important for power budgeting. Quiescent current is
  negligible regardless of exact variant; only matters later during analog-stage circuit
  design (dropout voltage, output tolerance), not for rail sizing.
- **Quectel idle-current table** — not important. Rail sizing is driven by the verified
  2-3A transient spec, not the idle figure.
- **W5500 max current (no manufacturer figure published)** — real gap, handled the standard
  industrial way: conservative derating now (132mA typical → budgeted 200mA, already folded
  into the 3.3V-LOGIC rail's ~1A target with its own 20-30% margin on top), closed empirically
  later rather than chased on paper now.

**Commissioning test item (tracked for Rev-A bring-up):** measure actual W5500 current draw
under worst-case conditions (100M link, continuous transmit) with a bench current probe;
confirm it falls within the 200mA budgeted margin. This is the standard way an unpublished
datasheet max gets closed out — margin at design time, measurement at hardware bring-up.

---

## Step 3 results: grounding & isolation architecture (2026-08-30)

### Isolation boundary map (from teardown + datasheet facts, not assumed)
| Interface | Galvanically isolated? | How |
|---|---|---|
| Digital inputs (8x) | **Yes** | LTV-247 optocouplers (3750 Vrms rated) |
| RS485 | **Yes** | Mornsun TD321S485H-A isolated module |
| Ethernet | **Yes** | Magjack's integrated transformer (Würth 7499010441) |
| Relay outputs (4x) | **Yes, on the switched-load side only** | Mechanical relay contacts (ALDP105) — the coil/control side is NOT isolated from logic ground |
| Power input | **No** | P-MOSFET reverse-polarity protection is not galvanic isolation — field power return is directly connected to system ground |
| Analog input | **Yes (added, improvement over reference)** | Reference board has none; we're adding an isolation amplifier/isolated ADC — decision finalized below |
| Analog output | **No (unchanged for now)** | Same non-isolation as the reference board; same reasoning as the input arguably applies here too — revisit when the analog subsystem (#6) is actually designed, not decided now |
| RS232 | **No** | MAX3232E has no isolation |
| LTE / WiFi | N/A | No field-wiring connection, no isolation barrier needed |

### Ground domains defined
- **GND_LOGIC** — ESP32-S3, W5500, PCA9535PW, MAX3232E, relay coil-driver control side (transistor + flyback diode reference), regulator returns. The analog stage's ESP32-S3-facing side (digital/logic side of the isolation barrier) also lives here — see GND_ANALOG_ISO below for the field-facing side.
- **GND_ANALOG_ISO** — isolated field-side ground for the analog input, added as a deliberate improvement over the reference board (decided below). Powered by its own small isolated DC-DC supply, never joined to GND_LOGIC. Analog output stays non-isolated for now and shares GND_LOGIC, pending the step-6 analog subsystem design.
- **GND_FIELD_DI** — isolated field-side return for the 8 digital inputs, common across all 8 channels, isolated from GND_LOGIC by the optocouplers.
- **GND_RS485_ISO** — isolated bus-side ground, provided internally by the Mornsun module. Never joined to GND_LOGIC.
- **Chassis/frame ground** — bonded to the metal DIN-rail enclosure. Tied to GND_LOGIC (via the field power return) at a **single point** near the power entry, so surge/TVS protection has a low-impedance path to dump transients into the chassis without creating a ground loop elsewhere on the board.

### Board-to-board header implication
Only **GND_LOGIC** crosses the header between LTEBOARD and IOBOARD — the MCU only ever
touches the non-isolated side of every interface (optocoupler output transistor, RS485
module's isolated-UART-facing side, relay coil driver). GND_FIELD_DI and GND_RS485_ISO stay
local to IOBOARD and never cross the header. This also means the header needs enough GND_LOGIC
pins for return-current capacity on the digital signals, not just one.

### Decision — analog input isolation: ADDED (2026-08-30)
The reference board does not isolate the analog input (0-10V / 4-20mA) — confirmed with
medium-high, not absolute, confidence: this is based on photo teardown identifying only an
LDO (AME8808) and op-amp (LM2904) on that daughterboard, with no isolation component visible,
not on a traced schematic. Decision: **deviate from the reference here and add isolation**,
since an unisolated 4-20mA loop input is a real, common source of ground-loop and noise
problems in industrial deployments, and this project isn't bound to a like-for-like clone.

Cost of this decision, assessed before committing to it: one additional isolation
amplifier/isolated-ADC part (small BOM impact), one additional small isolated DC-DC supply
(new 4th rail, below), a firmware driver change later (external isolated ADC instead of the
ESP32-S3's built-in `analogRead()`), and a PCB clearance/slot at layout time (step 9) — same
class of layout requirement as the optocouplers and RS485 module already need. Contained
mostly to the analog subsystem (#6 on the build list), not a project-wide redesign.

**Analog output stays non-isolated for now** — not part of this decision, flagged for
revisit when subsystem #6 is actually designed.

### Rail count, finalized: 4, not 3
- **5V** — relay coils (~160 mA worst case)
- **3.3V-LOGIC** — ESP32-S3, W5500, PCA9535PW, MAX3232E, RS485 module, analog stage's
  logic-side (~650 mA worst-case peak, size module for ≥850 mA-1A)
- **3.3V-LTE** — dedicated, sized for Quectel's 2-3A transient spec
- **3.3V-ANALOG-ISO (new)** — small isolated supply for the analog input's field side.
  Load is light (a single isolation amp/ADC, low tens of mA) but must be electrically
  isolated from every other rail, not just separately regulated.

---

## Step 4 results: module selection (2026-08-30)

Real, currently-in-production parts confirmed via manufacturer datasheets — not invented
part numbers. Margin check against the step 1-2 load budget included.

| Rail | Part | Input range | Output current rating | Margin vs. budget | Notes |
|---|---|---|---|---|---|
| 3.3V-LOGIC | **Würth MagI3C-VDLM 171013801** (1A) | 3.5-38V (abs max 42V) — covers 10-30V with wide margin both ends | 1A continuous | ~650mA worst-case peak → ~54% headroom | Adjustable output (resistor divider), set to 3.3V |
| 3.3V-LTE | **Würth MagI3C-VDLM 171033801** (3A) | 3.5-38V (abs max 42V) | 3A continuous | Quectel's 2-3A transient sits *inside* this part's continuous rating, not just a peak spec | This is the same part number identified on the reference board during teardown — confirms Roltek sized this correctly for the same reason we are |
| 5V (relays) | **Würth MagI3C-VDLM 171013801** (1A) — same SKU as Logic rail, separate physical module, divider re-tapped to 5V | 3.5-38V | 1A | ~160mA worst case → ~84% headroom | Runs in efficient PFM mode at light load per datasheet, no minimum-load issue |
| 3.3V-ANALOG-ISO | **Recom R1SX-3.33.3-R** (3.3Vin/3.3Vout — corrected 2026-08-31, see note) | 3.3V regulated input (fed from the 3.3V-LOGIC rail, not raw field voltage) | ~300mA / 1W | Load is a single isolation amp/ADC, tens of mA — heavy margin | 1kVDC isolation standard, "/H" suffix option available for 3kVDC if the analog subsystem design (step 6) calls for it |

**Result: 2 unique SKUs cover 3 of the 4 rails** (171013801 used twice, in physically separate
module instances — once for Logic, once for 5V), plus 171033801 for LTE, plus the isolated
module for the analog rail. Efficient BOM, not over-fragmented.

### SPICE/simulation availability (relevant to step 8's known limitation)
Würth doesn't publish a strict downloadable SPICE model for MagI3C parts — they provide
**REDEXPERT**, a proprietary browser-based simulator (efficiency, ripple, thermal, transient),
confirmed live for both 171013801 and 171033801. This is usable for step 8's ngspice work only
indirectly — REDEXPERT itself isn't ngspice, so our simulation there will still use an
idealized behavioral model for the module (as already noted in step 6 of this plan), with
REDEXPERT used as a separate cross-check outside of ngspice.

### Thermal derating
Both MagI3C parts publish Iout-vs-ambient-temperature derating curves at Vin=12V and 24V —
directly usable for the enclosure ambient check flagged back in the original roadmap.

### Action item carried into step 5/7 (not a part-number issue)
171033801's own transient-response graph is labeled for a 10%→100% load step, not a sharp
RF PA current burst specifically. Recommend sizing extra output bulk capacitance on the
3.3V-LTE rail (near the Quectel module, matching what the real board's teardown showed —
bulk caps placed right at the modem) rather than relying on the regulator's transient
response alone. This is a schematic/BOM addition at step 7, not a different part choice.

---

## Step 5 results: protection design & sequencing (2026-08-30)

Real, currently-orderable parts. Two of our earlier teardown-based part guesses turned out
to be wrong on closer verification — corrected here rather than carried forward silently.

### Correction to earlier teardown identification
- **"PJ307U" was assumed to be Toshiba SSM3J307T.** On verification, SSM3J307T is a real
  part, but it's rated **Vds = -20V only** — no safety margin over our 30V max input, and it
  has no current DigiKey/Mouser stock. Not usable here regardless of what the real board
  actually has. Using **Toshiba SSM3J351R,LF** instead: -60V rating (2x margin over 30V),
  low Rds(on), same SOT-23-family footprint, in stock at DigiKey.
- **"WE-S 542501" does not resolve to a real Würth part number** in Würth's own catalog,
  DigiKey, Mouser, or Octopart — likely a misread board marking during the photo teardown,
  not an actual orderable part. Using **Würth WCAP-CSSA 8853522140011** (4.7nF, X1/Y2 safety
  rated, 250VAC) instead — a real, in-stock, correctly-rated EMI suppression cap for this use.

### Sequencing (fixed order from the input connector)
1. **Terminal block** (input connector, sized per step 6 wiring selection — not yet done)
2. **PTC resettable fuse — Littelfuse RXEF135**: 1.35A hold / 2.70A trip / 72V max.
3. **Reverse-polarity protection — SWITCHED (2026-08-30) to TI LM74610-Q1 ideal diode
   controller + CSD18531Q5A N-channel MOSFET (60V NexFET)**, replacing the earlier discrete
   Toshiba SSM3J351R,LF P-MOSFET + Zener approach. Reasoning: the discrete approach's
   Vgs-overvoltage problem (found in the last recheck) was fixed with a Zener clamp, but
   that's a workaround for a failure mode this IC doesn't have in the first place — LM74610-Q1
   drives an N-MOSFET with its own internally-regulated gate drive, independent of input
   voltage, so there's no Vgs-overvoltage vulnerability to guard against at all. Follows TI's
   own reference design **TIDUBP3A ("Reverse Battery Protection LM74610-Q1")**, which uses
   this exact IC + a similar-class N-MOSFET and is rated for automotive 12/24V systems with
   load-dump survival well past 30V — comfortably covers our 10-30V range.
   Sourcing checked before switching (per project decision: only switch if realistically
   sourceable) — both parts are active production, in stock at DigiKey and Mouser (which has
   a dedicated Turkey storefront), a realistic path even without confirmed local Turkish
   distributor stock.
4. **Surge/EMI — TI TVS3300 Flat-Clamp surge protection device** (33V standoff, 40V max
   clamp at 35A/8-20µs — see correction below for why this replaced the earlier two-stage
   SMBJ36A design) + **Würth WCAP-CSSA 8853522140011 Y-cap** (4.7nF, X1/Y2, 250VAC).
5. Into the four DC-DC modules from step 4.

Fuse-then-MOSFET-then-TVS (not TVS first) is a deliberate choice: if the TVS eventually
fails from cumulative surge energy, TVS diodes typically fail **shorted** — having the fuse
upstream means that failure blows the fuse and disconnects the fault, rather than leaving a
permanent short with no protection response. The trade-off is slightly slower surge-clamp
response than a TVS placed right at the connector — acceptable here given this isn't a
formal-certification design.

### Fuse sizing rationale (shown, not just asserted)
Fuses are thermal/slow-acting — sized against realistic **sustained** current, not
millisecond-scale bursts (those are handled by the DC-DC modules' own current limiting and
local bulk capacitance, not the input fuse). Estimated sustained output power, using typical
(not instantaneous-peak) figures:
- 3.3V-LOGIC: ~400mA × 3.3V ≈ 1.3W
- 3.3V-LTE: ~500mA × 3.8V ≈ 1.9W (assumption — Quectel's real sustained active-session
  current wasn't fully verified earlier; flagged, not invented as fact)
- 5V: 160mA × 5V ≈ 0.8W (all 4 relays energized)
- 3.3V-ANALOG-ISO: ~0.1W (negligible)

Total ≈ 4.1W output. At ~85% typical buck efficiency, input power ≈ 4.8W. At minimum input
voltage (10V, worst case for max input current): **I_in ≈ 0.48A**. RXEF135's 1.35A hold
current (at 20C) gives ~2.8x margin above this estimate.

**Correction (2026-08-30, final recheck): PTC fuses derate with temperature, and this
matters at our assumed 60C ambient.** RXEF135's hold current drops from 1.35A at 20C to
**0.85A at 60C** per its datasheet derating curve. Re-checking margin at the worst-case
ambient, not room temperature: 0.85A / 0.48A ≈ **1.77x margin** — still adequate (comfortably
above the commonly-used 1.5x minimum), but notably tighter than the room-temperature number
implied. Using the 60C-derated figure going forward, not the 20C one, since that's the
realistic worst case for this design's assumed operating range.

### Open item carried to commissioning (same pattern as the W5500 item)
The 3.3V-LTE sustained-current assumption (500mA) above is an estimate, not a verified
figure. **Commissioning test item**: measure actual sustained input current under a real
LTE data session at Rev-A bring-up; confirm the fuse choice still holds with adequate margin.

### Correction (2026-08-30): single-TVS surge stage was not adequate — fixed
On re-check, the SMBJ36A's own clamping voltage (58.1V, at its rated surge current) exceeds
the MagI3C modules' absolute maximum input rating of **42V**. A surge event could expose the
modules to a voltage above what they're rated to survive, even with the TVS present.

This isn't a simple part swap — verified that every silicon TVS diode in the 33-36V standoff
class clamps at roughly **1.6x its standoff voltage**, regardless of package or power rating
(checked Littelfuse's 600W/SMBJ, 1500W/SMCJ, and 5000W/5KP families — all three hit the same
clamp voltage for a given standoff; a bigger die doesn't lower the ratio enough at this
voltage class). So a bigger or different single TVS does not fix this.

**Fix: coordinated two-stage clamp** (standard practice for exactly this mismatch, same
principle as cascaded surge-protective-device coordination):
1. **SMBJ36A stays** as the primary energy-absorbing diverter, right after the connector
   (unchanged from above).
2. **Add a small ferrite bead** between that node and each DC-DC module's input — negligible
   DC resistance (no meaningful voltage drop on the sustained 10-30V line), but enough
   high-frequency impedance to slow the surge edge and limit the current reaching stage 3.
3. **A second, smaller-Ipp TVS at each module's input pins** — seeing much less current than
   the primary diode already absorbed, it clamps much closer to its own breakdown voltage
   (not its full-current rating), bringing the actual module input excursion down under 42V.

Exact secondary TVS and ferrite bead part numbers are **not finalized yet** — the real
current apportionment between the two stages depends on impedances and timing that should be
verified in ngspice (step 8's surge simulation), not calculated by hand with false
confidence. Tracked as an open item for step 8, not a part-number gap to fill blindly now.

### Correction (2026-08-31): 3.3V-ANALOG-ISO module part number corrected
When this rail was originally specced (step 4), the part number was recorded as
Recom R1SX-3305, with an explicit open item noting "output voltage (5V here) is
provisional... may need the 3.3V-out sibling instead" — this was flagged honestly at
the time, not missed. Now wiring this rail in schematic capture: R1SX-3305 is
**3.3V-in, 5.0V-out** (Recom's R1SX naming encodes input then output voltage), which
doesn't match a rail named 3.3V-ANALOG-ISO. The correct part is **Recom
R1SX-3.33.3-R** — 3.3V-in, 3.3V-out, ~300mA, same 1kVDC isolation, same package/pinout
family. Mornsun B3305S-1WR3 (the alternate second-source) has the identical naming
mismatch and needs the same correction if it's ever actually used — not fixed here
since the primary part is what's going in the schematic.

### Correction (2026-08-31): two-stage clamp replaced — TI TVS3300 (Flat-Clamp technology)
Found a better solution while sourcing an application note for the surge stage: TI makes a
family of active-feedback "Flat-Clamp" surge protection devices (TVS0500 through TVS5800,
one device per standoff-voltage class) that clamp far closer to their standoff voltage than
a conventional silicon avalanche TVS does — a fundamentally different clamping mechanism
(internal feedback holds the voltage flat right above breakdown; dynamic resistance
30-50mΩ), not just a bigger/different part in the same class SMBJ36A belongs to.

Checked every family member for a fit:
- **TVS4000** (40V standoff — the closest match to SMBJ36A's old 36V margin) clamps at
  **50.3V**, still above the MagI3C modules' 42V absolute max. Doesn't solve the problem
  despite better standoff margin.
- **TVS3300** (33V standoff) clamps at **40V max** at its full 35A/8-20µs rating — under the
  42V limit, with 2V to spare. This is the only Flat-Clamp family member that satisfies our
  actual constraint (module input voltage < 42V during a surge event).

**Decision: replace the entire two-stage clamp (SMBJ36A + ferrite bead + secondary TVS) with
a single TVS3300**, placed at the same point in the sequence (after the reverse-polarity
stage, before the four DC-DC modules). This removes the two still-open, unfinalized parts
(ferrite bead and secondary TVS) from the step-8 simulation backlog entirely — one real,
fully-specified part instead of two unknowns.

Trade-off, stated honestly: TVS3300's 33V standoff gives ≈10% margin over our 30V max
continuous input, versus SMBJ36A's 36V standoff giving ≈20% margin. Clamp voltage under 42V
is the harder constraint (protects the modules from actual damage), so this trade is
accepted, but it's a real trade, not a free upgrade — worth reconfirming against the
project's actual max continuous input voltage requirement when that's finalized.

Sourcing checked before switching (same policy as the LM74610-Q1 switch): TVS3300DRVR is
active production, in stock at both DigiKey and Mouser, both of which ship to Turkey
(DigiKey has an authorized Turkish reseller, EKOM; Mouser ships internationally).

Package: SON-6 (DRV), same footprint style already used for other SON parts in this design.
Pinout: pins 4/5/6 = IN (tie together, to the protected-rail node), pins 1/2/3 + exposed
thermal pad = GND (tie together, to GND_LOGIC). No series/parallel components needed — it's
a direct two-node shunt device, per TI's own typical application circuit.

Temperature range: TVS3300's absolute max ratings were not independently re-pulled from its
own datasheet at this-writing — assumed consistent with the -40C to +125C operating /
-65C to +150C storage range documented for TVS0500 (same family, same datasheet
document/package), which comfortably covers our -20C to +60C assumed environment, but this
assumption should be confirmed against TVS3300's own datasheet page before layout freeze.

### Step 8 (2026-08-31): inrush current at power-up — simulated, resolved, no NTC needed
Simulated in ngspice rather than hand-waved. Circuit: source stepped 0V->30V (max design
input, worst-case hot-plug connection) through Rsrc (source/wiring impedance) + Rfuse (F1,
RXEF135's datasheet Rmin = 0.12ohm cold resistance -- the conservative lower bound, since a
lower fuse resistance gives a higher, more conservative inrush estimate) + Rq1 (Q1,
CSD18531Q5A's typical RDS(on) = 3.5mohm) + an assumed 0.05ohm PCB trace/connector parasitic,
into the input capacitor bank: 8x 4.7uF (2 per module x4 modules = 37.6uF total), each with a
representative 20mohm ceramic MLCC ESR.

Two cases run:
- **Worst case** (Rsrc = 10mohm, near-zero source impedance -- a very pessimistic assumption
  since the actual field supply's impedance is unknown): peak inrush **161A**, decaying with
  time constant tau ~= 6.9us (matches hand-calc RC = R_total x C_total, cross-checked).
- **More realistic case** (Rsrc = 0.5ohm, typical of a real supply/cable/screw-terminal
  contact resistance): peak inrush **44A**, tau ~= 25.3us.

Checked against real limits, not just eyeballed:
- Q1's datasheet gives IDM (pulsed drain current) = **400A for pulse width <=100us, duty
  cycle <=1%**. Worst-case 161A peak, ~35us total decay (5x tau), single non-repetitive
  event -- well within rating, ~2.5x margin even in the pessimistic case.
- F1 is a PTC (resettable) fuse -- its trip mechanism is thermal, with a datasheet-quoted
  9.6s time-to-trip at rated fault current. A ~35us current pulse carries far too little
  I^2t energy (~161^2 x 35e-6 ~= 0.9 A^2s) to meaningfully heat the PTC's thermal mass --
  no nuisance-trip risk. Not independently cross-checked against a surge-current
  rating specific to RXEF135 (not published/found), but the physics (thermal time constant
  in the seconds range vs. a microsecond-scale event) makes this a low-risk conclusion, not
  a hand-wave.

**Decision: no NTC thermistor needed.** The fuse's own cold resistance already provides
enough current-limiting that the MOSFET and fuse both have solid margin under the
worst-case assumption. Adding an NTC would only be justified if either limit were being
approached, which they aren't. One caveat carried forward, not fully closed: 161A even for
~35us is worth a sanity check against PCB trace/connector current-carrying capacity at
layout time (not a schematic-level concern, standard copper generally handles microsecond
pulses like this without issue, but flagging so it isn't silently assumed).

### Confirmed (2026-08-30, final recheck): MagI3C modules have built-in overcurrent/short-circuit protection
Checked directly against both 171013801 and 171033801 datasheets — both explicitly state
cycle-by-cycle current limiting via peak-current-mode control, with a dedicated "Overcurrent
Protection (OCP) and Short Circuit Protection (SCP)" section. No external current-limiting
circuitry is needed on the module outputs for basic overcurrent/short-circuit protection —
this removes what was an open question, not a new requirement.

---

## Step 6 results: connector & wiring selection (2026-08-30)

### Wire gauge
Sizing rule: wire must comfortably carry more than the fuse's trip current (2.70A,
RXEF135) — the fuse protects the wire, which only works if the wire's ampacity exceeds the
fuse rating, not the other way around.

- **NFPA 70 (NEC) Table 310.15(B)(16)**: even its smallest tabulated gauge, 18 AWG, is rated
  14A at 90°C insulation — over 5x margin above 2.7A.
- **Engineering ToolBox AWG current-rating table** (PVC-insulated, single-core, 30°C ambient,
  a more conservative near-enclosure reference than the NEC bulk-cable table): 20 AWG = 6.0A,
  22 AWG = 5.0A.

**Chosen: 20 AWG stranded copper, 80-105°C insulation** for the terminal-block-to-PCB power
pair. ~2.2x margin over the fuse's trip current even by the more conservative table, and a
comfortable, standard gauge for a small DIN-rail device — not oversized to the point of being
impractical to terminate.

### Terminal block
**Phoenix Contact MC 1,5/2-ST-3,5** — PCB-mount pluggable terminal block, COMBICON MC series.
- 2-position (V+, V-) — sufficient, since chassis/frame ground is bonded mechanically through
  the DIN-rail mounting clip itself, not through a third wire on this connector (matches the
  single-point chassis bonding decided in step 3).
- 8A nominal current rating, 160V rating — both far above our 2.7A/30V requirement, standard
  practice to not run a connector near its rated limit.
- 3.5mm pin pitch, accepts 28-16 AWG wire (our chosen 20 AWG is within range).
- [Datasheet](https://www.mouser.com/datasheet/3/507/5/phoenix_contact_1840366_en.pdf)

### Creepage/clearance
30V is low voltage — PCB trace spacing and connector pitch requirements at this level are
governed by standards like IPC-2221, and are comfortably satisfied by both a 3.5mm terminal
pitch and normal PCB design-rule spacing at low pollution degree (an enclosed industrial
device). Not treating this as a driving constraint at this voltage — flagging it as checked,
not as something requiring special layout accommodation, unlike the higher-voltage isolation
barriers (optocouplers, RS485 module) which do need specific clearance attention at layout
time (step 9).

### Note: board-to-board header power pins — deferred, not skipped
The header carrying power (3.3V-LOGIC, 3.3V-LTE, 3.3V-ANALOG-ISO, 5V, and their returns)
between LTEBOARD and IOBOARD is part of the general board-to-board interconnect, addressed
as its own milestone in roadmap step 7 (~20+ signal lines total, not just power) — not
duplicated here. This step covers the field power INPUT connector only.

---

## Final recheck (2026-08-30): steps 1-6, before schematic capture

Full re-audit requested and completed. Two real issues found and fixed (Zener clamp, and
fuse temperature-derating correction above); one item flagged for step 8 simulation (inrush
current); one open question resolved in our favor (MagI3C OCP/SCP); one new foundational gap
found and flagged (operating temperature range was never actually decided as a requirement —
see the Scope section at the top).

### Operating temperature check (-20C to +60C ambient, assumed — see caveat above)
| Part | Datasheet range | Fits? |
|---|---|---|
| MagI3C 171013801 / 171033801 | -40C to +105C ambient | Yes, wide margin |
| Littelfuse RXEF135 (fuse) | -40C to +85C | Yes, but hold-current derating matters — see fuse sizing correction above |
| TI TVS3300 (TVS, replaces SMBJ36A — see 2026-08-31 correction) | -65C to +150C storage (family datasheet, not independently re-pulled for TVS3300 specifically) | Yes, wide margin |
| Toshiba SSM3J351R,LF (MOSFET) | -55C to +150C | Yes, wide margin — **sourcing caveat**: confirmed via a third-party datasheet mirror, not toshiba.com directly (that URL 404'd); worth a direct cross-check before final BOM lock |
| Würth WCAP-CSSA Y-cap | -55C to +125C | Yes, wide margin |
| Recom R1SX-3.33.3-R | -40C to +100C | Yes, wide margin |

### Genuinely still open after this recheck
1. **Operating temperature range itself needs a real decision from Ozcan**, not an assumed
   default — everything above was checked against -20C/+60C because that's a reasonable
   guess, not a confirmed requirement.
2. ~~Inrush current~~ — resolved 2026-08-31, simulated in ngspice: 161A worst-case peak,
   comfortably within Q1's 400A pulsed rating; no NTC needed (see step 8 section).
3. ~~Secondary TVS + ferrite bead part numbers for the coordinated surge clamp~~ — resolved 2026-08-31, replaced by a single TVS3300 (see correction).
4. ~~Feedback divider resistor values and recommended external caps for the MagI3C modules~~ —
   resolved during step 7 schematic capture, pulled from each module's own datasheet.
5. ~~Toshiba SSM3J351R,LF datasheet sourcing caveat~~ — moot, this part was superseded
   entirely by LM74610-Q1 + CSD18531Q5A (2026-08-30 correction), no longer in the design.

No further architectural gaps found on this pass. Steps 1-6 are otherwise considered solid.
