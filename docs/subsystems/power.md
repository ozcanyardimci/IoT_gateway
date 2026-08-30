# Power subsystem — build plan

Status: **not started** (roadmap step 4a). Simulation only, using pre-made DC-DC modules
(no custom-designed regulator, no breadboard bring-up).

## Scope
Reverse-polarity protection, overcurrent/surge protection, and 3.3V/5V regulation from a
10-30VDC field input, feeding both boards over the board-to-board header.

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
| Analog input/output | **No** (as built on the reference board) | LDO + op-amp stage shares ground with logic; ESP32-S3's own ADC does the conversion — no isolation barrier present |
| RS232 | **No** | MAX3232E has no isolation |
| LTE / WiFi | N/A | No field-wiring connection, no isolation barrier needed |

### Ground domains defined
- **GND_LOGIC** — ESP32-S3, W5500, PCA9535PW, MAX3232E, relay coil-driver control side (transistor + flyback diode reference), regulator returns, and the analog LDO/op-amp local ground (tied to GND_LOGIC at one point — see open item below).
- **GND_FIELD_DI** — isolated field-side return for the 8 digital inputs, common across all 8 channels, isolated from GND_LOGIC by the optocouplers.
- **GND_RS485_ISO** — isolated bus-side ground, provided internally by the Mornsun module. Never joined to GND_LOGIC.
- **Chassis/frame ground** — bonded to the metal DIN-rail enclosure. Tied to GND_LOGIC (via the field power return) at a **single point** near the power entry, so surge/TVS protection has a low-impedance path to dump transients into the chassis without creating a ground loop elsewhere on the board.

### Board-to-board header implication
Only **GND_LOGIC** crosses the header between LTEBOARD and IOBOARD — the MCU only ever
touches the non-isolated side of every interface (optocoupler output transistor, RS485
module's isolated-UART-facing side, relay coil driver). GND_FIELD_DI and GND_RS485_ISO stay
local to IOBOARD and never cross the header. This also means the header needs enough GND_LOGIC
pins for return-current capacity on the digital signals, not just one.

### Open decision — analog input isolation (need your call)
The reference board does **not** isolate the analog input (0-10V / 4-20mA), and we're
otherwise following its architecture. But an unisolated 4-20mA loop input is a common real
source of ground-loop and noise problems in actual industrial deployments — a professional
building this today, for an industrial product rather than a like-for-like clone, would
often add isolation here (e.g. an isolated ADC front-end or isolation amplifier). Two
options: (a) replicate as-is (non-isolated, matches the reference, simpler/cheaper), or
(b) add isolation on the analog input as a deliberate improvement over the reference design.
Your call — this affects the analog I/O subsystem's design later, but the ground architecture
above already accounts for either choice (GND_LOGIC assumption holds for (a); (b) would add
a fourth isolated domain, GND_ANALOG_ISO).
