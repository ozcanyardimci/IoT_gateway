# Ethernet Subsystem — Build Plan

**Status:** Schematic capture (Step 4) done, 2026-09-13. All 7 design decisions locked,
including the magjack termination question (decision 4) — now CONFIRMED, not just high
confidence: reading the actual Würth 7499010441 datasheet directly showed CTD/CRD are
terminated internally and pin 8 (GND) + both shield pins are one internal node, so all
three go to `EARTH` on this schematic. Schematic also picked up four intentional additions
beyond the original decision list, all pulled from real reference schematics and confirmed
sound: a ferrite bead splitting `3V3A` off `3V3_LOGIC` for the AVDD pins, a crystal
feedback/damping resistor pair, four 0Ω TX/RX line damping resistors (future-proofing, see
decision 4), a `GND_LOGIC`-to-`EARTH` bridging capacitor, and a second bulk capacitor
(one per 3.3V rail instead of one). Remaining: ERC (deferred project-wide, see `CLAUDE.md`)
and BOM (Step 7).

## Scope

One 10/100Mbps wired Ethernet port on **LTEBOARD** (not IOBOARD — see `CLAUDE.md`
architecture: W5500 lives with the MCU, not on the I/O board), built around the WIZnet
W5500 SPI-to-MAC+PHY chip driving an RJ45 jack with integrated magnetics. Not in scope:
exact MCU GPIO pin numbers for SPI/INT/RST (deferred project-wide to roadmap step 6, same
as every other subsystem's hierarchical labels), PCB layout differential-pair routing
rules (belongs at the layout stage), gigabit operation (the W5500 is 10/100-only — already
implicit in the chip choice made before this doc existed).

## Design approach

Seven decisions:

1. **MAC+PHY: WIZnet W5500, 48-pin LQFP.** Already the standing project choice
   (`architecture.md`, `power.md`) — not re-litigated here, just re-confirmed against the
   datasheet directly 2026-09-10 ([W5500 datasheet
   v1.1.0](https://docs.wiznet.io/img/products/w5500/W5500_ds_v110e.pdf)). Single 3.3V
   supply (2.97-3.63V), 132mA typical / no max published (already the exact figure and
   margin choice `power.md` made — 200mA budgeted, 1.3-1.5x typical — not changed here).
   Internal 1.2V core regulator, no external 1.2V rail needed. SPI Mode 0/3, MSB-first,
   guaranteed stable to 33.3MHz (80MHz theoretical max).
2. **Clock: external 25MHz crystal, 18pF||18pF load caps.** The W5500 needs an external
   clock source — either a crystal (XI/XO pins) or a 3.3V single-ended TTL oscillator into
   XI with XO floating. Went with a crystal (Y1, 25MHz, ±30ppm) plus two 18pF load caps,
   matching the datasheet's own spec exactly. Cross-checked against WIZnet's own published
   reference schematic (`w5500-ref-rj45with20150406.pdf`) — same crystal frequency, same
   18pF/18pF load caps, independently confirmed twice.
3. **Support passives, per datasheet + WIZnet's own reference schematic (both agree):**
   EXRES1 = 12.4kΩ 1% (pin 10 to AGND — sets an internal bias current, not optional),
   TOCAP = 4.7µF (pin 20 — datasheet says keep this trace short), 1V2O bypass = 10nF (pin
   22 — this is the internal 1.2V regulator's *output*, not a supply input; nothing else
   connects here besides the cap), plus standard 0.1µF decoupling at each VDD/AVDD pin and
   a 10µF/16V bulk cap on the 3.3V rail. All five values matched exactly between the
   datasheet's own spec and WIZnet's independently-published reference schematic — high
   confidence, two-source agreement.
   **Corrected during schematic capture (2026-09-13):** the real W5500 KiCad symbol breaks
   AVDD out on 6 pins (4, 8, 11, 15, 17, 21), not the 4 this doc originally listed — VDD is
   separately just pin 28 (see the corrected pinout table below). That means 7 decoupling
   caps are needed (6×AVDD + 1×VDD), which is what's on the schematic — the schematic was
   right, this doc's pin table was wrong, now fixed.
   **AVDD filtering — reference-schematic addition, not in the original decision list:**
   WIZnet support recommends filtering AVDD off the main 3.3V rail through a ferrite bead
   rather than tying it directly to `3V3_LOGIC`. Added FB1 between `3V3_LOGIC` and a new
   local net `3V3A` (AVDD pins only — VDD pins stay directly on `3V3_LOGIC`). Bead spec:
   0603, 120Ω @ 100MHz (90Ω min), DCR 200mΩ max, 1A max rated current (any bead meeting this
   works per WIZnet; sourcing equivalents: Murata BLM18PG121SN1D, TDK MPZ1608S121A).
   **Bulk cap — one per rail, not one total:** confirmed against two independent reference
   schematics (WIZnet's own, and the Cetus-magjack variant) that a 10µF/16V polarized bulk
   cap belongs on *each* 3.3V rail separately — one on `3V3A` (C_BULK1), one on the main
   `3V3D`/VDD rail (C_BULK2) — not a single shared bulk cap as this doc originally implied.
   Both use the KiCad `Device:C_Polarized` symbol family (Ozcan used the `_US` variant —
   functionally identical, different visual convention only), pin 1 = +, pin 2 = –.
   **Crystal bias/damping — reference-schematic addition:** WIZnet's reference schematic
   shows a 1MΩ feedback resistor across XI/XO (sets the oscillator's DC bias point) plus a
   0Ω series damping resistor in the XI path. Added both: R16 (1MΩ, across XI/XO) and R21
   (0Ω, series in the XI line) — traced on the schematic and confirmed in series/parallel
   as intended, not just placed nearby.
4. **Magnetics + RJ45: Würth Elektronik 7499010441 (WE-RJ45LAN), integrated transformer +
   RJ45 jack, one part.** Already the standing choice — this is what `power.md`'s
   isolation table cites for "Ethernet: Yes, isolated." Confirmed 2026-09-10 from Würth's
   own datasheet: 1:1 turns ratio (±2%), ≥1500V RMS isolation, 2 physical bi-color
   (green/yellow) integrated LEDs, IEEE 802.3u (100BASE-TX) compliant, through-hole,
   shielded housing.
   **Part number visually re-confirmed against the reference hardware, not just the
   datasheet pull:** photos of the actual reference LTEBOARD (`reference-photos/2026-09-
   08-hardware-assembly-photos/photo_22...jpg` and `photo_24...jpg`) show the jack's own
   case marking reading "WE 7499010441" directly — this is a match against the physical
   part on the real board, not just a part-number chosen from a datasheet search. Also
   visually confirmed: it's a standard single-port RJ45 shape (matches the enclosure's
   Ethernet cutout in `photo_27`, which is visibly larger and a different shape than the
   small rectangular terminal-block cutouts used for RS485/RS232/Analog on the same
   panel) — no ambiguity about connector type here, unlike RS232's DB9-vs-terminal-block
   question earlier.
   **Termination network — CONFIRMED 2026-09-13, no longer just high confidence.** Read the
   actual Würth 7499010441 datasheet directly (not a summary): its own internal schematic
   (page 2) shows CTD/CRD going through internal 75Ω/75Ω resistors per side, converging
   through a 0.001µF/2kV cap and two GDTs, to a combined internal node labeled "GND Shield"
   — i.e. pin 8 (GND) and both shield pins (S1, S2) are the *same* internal node. Cross-
   checked against two other real datasheets (Würth 7499011441A, and Cetus J1B1211CCD — the
   actual part in WIZnet's reference design) which show the identical internal topology.
   **Conclusion: no external Bob-Smith/termination components needed — CTD/CRD = No
   Connect. Pin 8 (GND) and both shield pins (S1, S2) all go to the SAME net, `EARTH` — do
   not split them between `GND_LOGIC` and `EARTH`,** since pin 8 and the shield are
   internally the same node and splitting them would short `GND_LOGIC` to `EARTH` through
   the connector. (This corrects an earlier draft of this guidance, given verbally before
   the real datasheet was read, that said pin 8 → `GND_LOGIC` and shield → `EARTH`
   separately — wrong, and fixed on the schematic before it caused a problem.)
   **Ground bridge cap — reference-schematic addition:** added a 1nF/2kV capacitor (C7)
   bridging `GND_LOGIC` to `EARTH`, matching an equivalent cap seen in a reference
   schematic (there labeled between "GND" and "CGND"). This gives a high-frequency
   common-mode/ESD return path between the two deliberately-separated ground domains
   without duplicating the magjack's own internal 1nF/2kV cap (different junction,
   different purpose) — keeps `GND_LOGIC`/`EARTH` split as separate nets everywhere else.
   **TX/RX line damping resistors — optional, added anyway as cheap future-proofing:** the
   reference schematic marks 0Ω series resistors on TXN/TXP/RXN/RXP between the W5500 and
   the magjack as an explicitly optional network. Added all four (R17-R20) as 0Ω
   placeholders — electrically transparent at 0Ω, but if a signal-integrity/EMI issue shows
   up at bring-up, a 0Ω placeholder can be swapped for a real value in minutes; without the
   footprint, the same fix needs an unreliable trace-cut rework or a board respin. Traced on
   the schematic and confirmed genuinely in series (U3 pin → resistor → `TXN`/`TXP`/`RXN`/
   `RXP` net → magjack), not shorted across by a same-named label on both sides.
   **External CT bias/termination network — explicitly rejected.** A different reference
   schematic variant shows a center-tap bias+termination network (49.9Ω/49.9Ω/10Ω resistors
   + caps) built externally around a *bare, unterminated* transformer magjack. That's a
   different magjack topology than the Würth 7499010441, which already terminates CTD/CRD
   internally with no externally-accessible raw center-tap node — this network does not
   apply here and was not added.
5. **LED pairing: LINKLED + ACTLED, the other two spare.** The W5500 exposes 4 open-drain
   active-low status outputs — SPDLED (pin 24), LINKLED (pin 25), DUPLED (pin 26), ACTLED
   (pin 27) — but the magjack only has 2 physical LED positions. Wired LINKLED (link
   established, active low = on) to one position and ACTLED (traffic present, active low
   = on) to the other — the conventional pairing for a 2-LED magjack, and the pair a
   technician actually needs to see at a glance (link up? traffic moving?). SPDLED and
   DUPLED left unconnected — same "spare, intentionally unused" pattern already used for
   RS232's channel 2 and the analog subsystem's second op-amp. Current-limit resistor:
   330Ω per LED, from `3V3_LOGIC` through the LED to the W5500's open-drain pin — reused
   from WIZnet's own reference schematic value; against Würth's LED Vf (1.8-2.4V @ 20mA
   rated), 330Ω gives roughly 3-4.5mA — a visible indicator current well under the 20mA
   rating, consistent with how this project already under-drives the 6 status-indication
   LEDs relative to their max rating.
6. **Control lines: dedicated SPI bus + dedicated INTn + GPIO-controlled RSTn.** SCSn,
   SCLK, MOSI, MISO form a dedicated SPI bus (not shared with anything else — matches
   `architecture.md`'s existing bus table, which already lists Ethernet as the only SPI
   consumer). INTn (active low) goes to its own MCU GPIO, same as the datasheet's
   interrupt-driven usage model. RSTn (active low, needs ≥500µs low pulse per datasheet,
   internal pull-up already present) is wired to a dedicated MCU GPIO rather than a
   passive RC-only power-on reset — this is a real design choice, not a datasheet
   requirement: a GPIO lets firmware recover a hung W5500 without a full board power
   cycle, matching this project's existing preference for GPIO-observable/controllable
   signals over purely hardwired ones (digital inputs, relay outputs). Open to revisiting
   if pin budget on the MCU gets tight once every subsystem's GPIO needs are tallied at
   roadmap step 6. PMODE0/1/2 (pins 43-45) left floating — datasheet says these have
   internal pull-ups and default to auto-negotiation mode, no override needed.
7. **Power: `3V3_LOGIC` / `GND_LOGIC`, already budgeted — no new decision.** W5500's
   132mA typical / 200mA budgeted figure is already itemized in `power.md`'s 3.3V-LOGIC
   rail sizing (it's literally already on that rail's load list). Nothing changes here;
   this decision just confirms there's no new power-budget work for this subsystem.

## Step 1 results: front end (drafted, 2026-09-10)

| Parameter | Value | Source |
|---|---|---|
| Chip | WIZnet W5500, 48-pin LQFP (7x7mm, 0.5mm pitch) | [W5500 datasheet](https://docs.wiznet.io/img/products/w5500/W5500_ds_v110e.pdf) |
| Supply | 3.3V (2.97-3.63V), 132mA typ / no max published, 200mA budgeted | Datasheet — already in `power.md`'s rail sizing, unchanged |
| Crystal | 25MHz, ±30ppm, 18pF||18pF load caps (Y1, C13, C14) | Datasheet + WIZnet reference schematic (2-source agreement) |
| EXRES1 | 12.4kΩ 1%, pin 10 to AGND | Datasheet + WIZnet reference schematic (2-source agreement) |
| TOCAP | 4.7µF, pin 20, short trace | Datasheet + WIZnet reference schematic (2-source agreement) |
| 1V2O bypass | 10nF, pin 22 (internal 1.2V regulator output — no external supply here) | Datasheet |
| General decoupling | 0.1µF at each of 7 VDD/AVDD pins + 1x 10µF/16V bulk cap per 3.3V rail (2 total) | Datasheet + WIZnet's general hardware design guide, corrected 2026-09-13 (see decision 3) |
| AVDD filtering | Ferrite bead (FB1) between `3V3_LOGIC` and local net `3V3A` (AVDD pins only) | WIZnet support, added 2026-09-13 (see decision 3) |
| Crystal bias/damping | R16 = 1MΩ across XI/XO, R21 = 0Ω series in XI path | WIZnet reference schematic, added 2026-09-13 (see decision 3) |
| TX/RX line damping | R17-R20 = 0Ω series on TXN/TXP/RXN/RXP (optional, future-proofing) | WIZnet reference schematic, added 2026-09-13 (see decision 4) |
| Ground bridge | C7 = 1nF/2kV, `GND_LOGIC` to `EARTH` | Reference schematic, added 2026-09-13 (see decision 4) |
| SPI | Mode 0/3, MSB-first, guaranteed to 33.3MHz (80MHz theoretical) | Datasheet |
| Interrupt | INTn, active low, dedicated GPIO | Datasheet |
| Reset | RSTn, active low, ≥500µs low pulse, internal pull-up, dedicated GPIO (decision 6) | Datasheet |
| PMODE0-2 | Floating (internal pull-up, default auto-negotiation) | Datasheet |

**Relevant pinout** (48-LQFP, confirmed 2026-09-10):

| Pin(s) | Name | Function |
|---|---|---|
| 1, 2 | TXN, TXP | Ethernet transmit differential pair, to magjack |
| 5, 6 | RXN, RXP | Ethernet receive differential pair, to magjack |
| 10 | EXRES1 | External reference resistor (12.4kΩ 1% to AGND) |
| 4, 8, 11, 15, 17, 21 | AVDD | Analog 3.3V supply (corrected 2026-09-13 — confirmed against the real KiCad symbol, not the 4-pin list this table originally had) |
| 28 | VDD | Digital 3.3V supply (separate pin, not shared with AVDD) |
| 3, 9, 14, 16, 19, 48 | AGND | Analog ground |
| 29 | GND | Digital ground |
| 20 | TOCAP | Reference capacitor (4.7µF) |
| 22 | 1V2O | Internal 1.2V regulator output (10nF cap only) |
| 24 | SPDLED | Speed LED, active low (used: no, decision 5) |
| 25 | LINKLED | Link LED, active low (used: yes, decision 5) |
| 26 | DUPLED | Duplex LED, active low (used: no, decision 5) |
| 27 | ACTLED | Activity LED, active low (used: yes, decision 5) |
| 30, 31 | XI/CLKIN, XO | 25MHz crystal |
| 32 | SCSn | SPI chip select, active low |
| 33 | SCLK | SPI clock |
| 34 | MISO | SPI data out (high-Z when SCSn high) |
| 35 | MOSI | SPI data in |
| 36 | INTn | Interrupt output, active low |
| 37 | RSTn | Reset input, active low |
| 43, 44, 45 | PMODE2, PMODE1, PMODE0 | PHY mode select, leave floating |

## Step 2 results: magnetics + RJ45 (drafted, high confidence — 2026-09-10)

**Würth Elektronik 7499010441 (WE-RJ45LAN)** — integrated transformer + RJ45 jack, single
part, through-hole, shielded. 1:1 turns ratio (±2%), ≥1500V RMS isolation, 2 physical
bi-color (green/yellow) LEDs, IEEE 802.3u compliant. Part number and physical form factor
visually re-confirmed against the reference hardware's own LTEBOARD photos (case marking
reads "WE 7499010441" directly) — not just a datasheet-search pick.

**Termination network: CONFIRMED, not just high confidence** — see decision 4 for the full
finding. Read the actual 7499010441 datasheet directly: CTD/CRD are internally terminated
(75Ω/75Ω + 0.001µF/2kV + GDTs) into a combined "GND Shield" node, so pin 8 + both shield
pins are one net. No external Bob-Smith network needed; CTD/CRD = No Connect; pin 8 + S1 +
S2 all → `EARTH`.

## Step 3 results: LED + control lines (drafted, 2026-09-10)

LINKLED (pin 25) and ACTLED (pin 27) each through a 330Ω resistor from `3V3_LOGIC` to one
of the magjack's 2 LED positions; SPDLED (pin 24) and DUPLED (pin 26) left unconnected.
SCSn/SCLK/MOSI/MISO to dedicated MCU GPIOs (exact numbers at roadmap step 6). INTn to a
dedicated MCU GPIO. RSTn to a dedicated MCU GPIO (decision 6 — firmware-controlled reset,
not RC-only). PMODE0-2 left floating.

## Step 4: schematic capture — DONE, 2026-09-13 (Ozcan, hands-on)

Wired in KiCad: `hardware/kicad/lteboard/lteboard/ethernet.kicad_sch`, same project as
`core-compute.kicad_sch`. Reviewed file-level 2026-09-13 (component list, label/net
connectivity traced via symbol placement + wire coordinates, not just visual inspection):

- U3 (W5500), T1 (magjack, `WE-RJ45_7499010441`), Y1 (25MHz crystal), C5/C6 (18pF load
  caps), R-EXRES1 (12.4kΩ), C-TOPAC1 (4.7µF), C-1V2O_1 (10nF), FB1 (ferrite bead),
  R-LED1/R-LED2 (330Ω), C-dec1 through C-dec7 (0.1µF ×7, one per VDD/AVDD pin),
  C_BULK1/C_BULK2 (10µF/16V polarized, one per 3.3V rail), R16 (1MΩ, crystal feedback),
  R21 (0Ω, crystal-side damping), R17-R20 (0Ω, TX/RX line damping) — all present and
  correctly connected.
- Hierarchical labels: `3V3_LOGIC` (×3), `ETH_MOSI`/`ETH_SCLK`/`ETH_RSTN`/`ETH_SCSN` (input
  shape), `ETH_MISO`/`ETH_INTN` (output shape) — all match the root sheet's `lteboard.
  kicad_sch` sheet-pin block exactly, no shape mismatches.
- Global labels: `GND_LOGIC` (×6) and `EARTH` (×2, per Ozcan's own call to make it a global
  label given other subsystems use it too) — traced the magjack's pin8/S1/S2 node and C7's
  two pins to confirm both `EARTH` instances and the `GND_LOGIC`-`EARTH` bridge are wired
  as intended (see decision 4).
- **One typo found, not yet fixed:** C7's Value field reads `1nF/2kW` — should be `1nF/2kV`
  (kilovolt, not kilowatt; "2kW" isn't a real capacitor voltage rating and would confuse a
  distributor search). Doesn't affect the netlist, just the label — fix before BOM/ordering.
- No other errors found. Net effect: Step 4 is complete and correct pending that one label
  fix.

## Step 5: verification checklist / ERC — not run yet

Deferred to the single end-of-project ERC pass (see `CLAUDE.md` open items) — same policy
now applied to every subsystem, RS232 onward.

## Step 6: acceptance criteria — draft, to confirm once Step 4 is done

1. Logic-side operation from `3V3_LOGIC` draws within the already-budgeted 132mA typ /
   200mA margin — no change needed in `power.md`.
2. Link establishes and negotiates correctly with a real 10/100 switch/device — bench
   item.
3. LINKLED/ACTLED indicate correctly (link state, traffic) — bench item.
4. SPI communication verified from firmware at a rate at or below the 33.3MHz guaranteed
   minimum — bench item, can't close on paper.
5. If a bench-level signal-integrity issue on the Ethernet link ever points back at
   termination or line damping, the R17-R20 0Ω placeholders can be swapped for a real
   value without a board respin (see decision 4) — termination itself (decision 4) is now
   datasheet-confirmed, not just high confidence.

All 5 items require real hardware — Rev-A bring-up.

## Step 7: documentation & BOM — BOM not written yet

`hardware/datasheets/README.md`'s Ethernet section updated 2026-09-13 (termination now
confirmed, ferrite bead source added). `hardware/bom/ethernet_bom.csv` still to write —
now unblocked since real reference designators exist on the schematic (U3, T1, Y1, C5, C6,
C7, C_BULK1, C_BULK2, C-dec1 through C-dec7, C-1V2O_1, C-TOPAC1, R-EXRES1, R-LED1, R-LED2,
R16-R21, FB1).

## Step 8: sign-off — not yet

Pending Step 7 (BOM) and the end-of-project ERC pass.

## Steps

1. **Front end: W5500, crystal, support passives** — **DONE (this doc)**.
2. **Magnetics + RJ45 selection** — **DONE (this doc)** — Würth 7499010441, part number
   and connector type visually re-confirmed against reference-hardware photos; termination
   network CONFIRMED integrated, no external network, via direct datasheet read (decision
   4).
3. **LED + control line assignment** — **DONE (this doc)**.
4. **Schematic capture (KiCad)** — **DONE, 2026-09-13** — reviewed file-level, one label
   typo found (C7 value), otherwise complete and correct.
5. **Verification checklist / ERC** — Deferred to the single end-of-project pass.
6. **Acceptance criteria** — Drafted (this doc), confirmed applicable now that Step 4 is
   done.
7. **Documentation & BOM** — Datasheets README done; BOM CSV not written yet.
8. **Sign-off** — Not started, pending Step 7 and ERC.

## Revision history

| Date | Change |
|---|---|
| 2026-09-10 | Doc created. Steps 1 and 3 locked from the W5500 datasheet, cross-checked against WIZnet's own published reference schematic (crystal, EXRES1, TOCAP, 1V2O, LED resistor values all independently confirmed twice). Step 2 (magnetics/RJ45) drafted but not fully locked — Würth 7499010441's datasheet appears to show integrated GDT + integrated Bob-Smith termination, which would make WIZnet's externally-added termination network redundant, but the automated datasheet read couldn't produce a clean confirmed pin diagram; flagged as an explicit open item to resolve before Step 4 wiring, not guessed either way. RSTn wired to a dedicated GPIO (decision 6) — a deliberate design choice for firmware-recoverable resets, not a datasheet requirement, open to reconsideration at roadmap step 6 if MCU pin budget is tight. |
| 2026-09-10 | **Resolved decision 4 using the reference-hardware photos**, per Ozcan's reminder that the reference teardown photos and this project's earlier component decisions are a source to check before treating something as unresolved. Found the actual magjack on the reference LTEBOARD (`photo_22`, `photo_24`): case marking reads "WE 7499010441" — an exact, direct part-number match against the physical reference part, not just a datasheet-search pick. Also checked the board's own trace layout around the jack for termination-network evidence: no discrete resistor/capacitor cluster sits next to the connector (where an external Bob-Smith network would have to be, for signal-integrity reasons, if one existed) — consistent with the datasheet's own suggestion that termination is built into the part. Combined, this raises decision 4 to "high confidence, not blocking" rather than "unresolved." Step 2 promoted to DONE; Step 4 (schematic capture) no longer blocked. Also confirmed via the same photos that the enclosure's Ethernet cutout (`photo_27`) is RJ45-shaped, distinct from the small terminal-block cutouts used elsewhere on the panel — no connector-type ambiguity for this subsystem. Added the Ethernet subsystem section to `hardware/datasheets/README.md` ahead of schematic capture (documentation-first, since the part choice is now locked). |
| 2026-09-13 | **Schematic capture (Step 4) completed by Ozcan and reviewed file-level.** Four intentional additions beyond the original decision list, all pulled from real reference schematics and confirmed sound: ferrite bead (FB1) splitting a `3V3A` net off `3V3_LOGIC` for the AVDD pins (WIZnet support recommendation); crystal feedback resistor R16 (1MΩ, across XI/XO) + damping resistor R21 (0Ω, series in XI path); four 0Ω TX/RX line damping resistors R17-R20 (datasheet marks these optional; added as cheap future-proofing — a 0Ω placeholder can be swapped for a real value later without a board respin); a second 10µF/16V bulk cap (one per 3.3V rail instead of one shared). **Corrected this doc's AVDD pin table:** the real W5500 KiCad symbol has AVDD on 6 pins (4, 8, 11, 15, 17, 21), not the 4 originally listed (15, 17, 21, 28) — VDD is separately pin 28. This doc's table was wrong; Ozcan's 7-cap decoupling layout was right. **Upgraded decision 4's termination-network conclusion from "high confidence" to CONFIRMED:** read the actual 7499010441 datasheet directly (not a summary) — its own schematic shows CTD/CRD terminated internally (75Ω/75Ω + 0.001µF/2kV + GDTs) into a combined "GND Shield" node, cross-checked against two more real datasheets (Würth 7499011441A, Cetus J1B1211CCD) showing the same topology. **Corrected the shield/GND net assignment:** pin 8 (GND) and both shield pins (S1, S2) are the same internal node and must all go to `EARTH` — an earlier verbal answer (before the real datasheet was read) said to split them between `GND_LOGIC` and `EARTH`, which would have shorted the two ground domains together through the connector; caught before it caused a problem, and confirmed fixed on the schematic. Added a `GND_LOGIC`-`EARTH` bridge capacitor (C7, 1nF/2kV) per a reference schematic showing the same. Explicitly rejected a different reference variant's external center-tap bias/termination network (49.9Ω/49.9Ω/10Ω + caps) — that network is for a bare-transformer magjack, not applicable to the Würth 7499010441's internally-terminated construction. **One typo found, not yet fixed:** C7's Value field reads `1nF/2kW`, should be `1nF/2kV` — cosmetic only, fix before BOM/ordering. Updated `CLAUDE.md`, `docs/roadmap.md`, and `hardware/datasheets/README.md` to match. |
