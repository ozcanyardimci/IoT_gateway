# Ethernet Subsystem — Build Plan

**Status:** In progress. Steps 1-3 (front end, magnetics/protection, LED/control lines)
locked 2026-09-10, including the magjack termination question (decision 4) — resolved
with high confidence against both the datasheet and the reference-hardware photos, not
fully pin-level-confirmed but no longer blocking. Step 4 (schematic capture) is next —
hands-on step, done by Ozcan in KiCad.

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
   one 10µF bulk cap on the 3.3V rail. All five values matched exactly between the
   datasheet's own spec and WIZnet's independently-published reference schematic — high
   confidence, two-source agreement.
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
   **Termination network — more confident now, still not 100% locked.** The datasheet
   read (twice, independently) surfaced what looks like an integrated GDT (gas discharge
   tube) + integrated 75Ω common-mode termination resistors + an integrated 1nF/2kV
   center-tap capacitor built into this part — i.e. it may already contain the entire "Bob-
   Smith termination" network that WIZnet's own reference schematic otherwise builds
   *externally* on the PCB (4×49.9Ω resistors + 1×1nF/2kV cap next to a bare-transformer
   magjack). Checked the reference board's own layout for corroborating physical evidence:
   in `photo_22`/`photo_24`, the traces leaving the jack's pins run directly toward the
   rest of the board with no visible discrete resistor/capacitor cluster sitting next to
   the connector itself — which is where an external Bob-Smith network would have to sit
   for it to do its job (termination has to be physically close to the magnetics, not
   dangling on long traces back to the chip). That's consistent with the datasheet finding
   and raises confidence the termination is built into the part. It isn't a 100% pin-level
   confirmation, though — I still couldn't get a clean pin-by-pin diagram out of the
   datasheet PDF, and can't fully rule out the network sitting somewhere off-frame in the
   photos. **Net effect: don't add WIZnet's external 4×49.9Ω + 1nF/2kV network by default**
   — the weight of evidence (datasheet text + real-board layout) is against needing it —
   but do a final visual check against the part's own pinout diagram before soldering, in
   case that changes. Center-tap/shield bonding, if the datasheet's own pin diagram calls
   for it, uses this project's established `EARTH` net (same label RS485's GDT third
   electrode bonds to).
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
| General decoupling | 0.1µF at each VDD/AVDD pin + 1x 10µF bulk on 3.3V | Datasheet + WIZnet's general hardware design guide |
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
| 15, 17, 21, 28 | AVDD | Analog 3.3V supply |
| 28 | VDD | Digital 3.3V supply (shared pin number with AVDD per datasheet) |
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

**Termination network:** evidence now points toward "built into the part, don't add
WIZnet's external network" — both the datasheet text (GDT + 75Ω + 1nF/2kV all read as
inside the part) and the reference board's own layout (no discrete R/C cluster next to
the jack in the photos, which is where an external network would have to sit) agree. Not
a 100% pin-level confirmation — do a final visual check against the part's own pinout
diagram before soldering — but this is no longer a hard blocker on starting Step 4.

## Step 3 results: LED + control lines (drafted, 2026-09-10)

LINKLED (pin 25) and ACTLED (pin 27) each through a 330Ω resistor from `3V3_LOGIC` to one
of the magjack's 2 LED positions; SPDLED (pin 24) and DUPLED (pin 26) left unconnected.
SCSn/SCLK/MOSI/MISO to dedicated MCU GPIOs (exact numbers at roadmap step 6). INTn to a
dedicated MCU GPIO. RSTn to a dedicated MCU GPIO (decision 6 — firmware-controlled reset,
not RC-only). PMODE0-2 left floating.

## Step 4: schematic capture — next up (Ozcan, hands-on)

Not started, but no longer blocked — decision 4/Step 2's termination question is resolved
with high confidence (don't add an external Bob-Smith network; see Step 2). Wire the
magjack's RJ45-side pins straight through to the W5500's TXN/TXP/RXN/RXP, do a quick
visual sanity check against the part's own pinout diagram for the LED and any center-tap/
shield pins as you place it, and flag me if anything on the physical part looks different
from what's documented here. Sheet: `hardware/kicad/lteboard/lteboard/ethernet.kicad_sch`, same project as
`core-compute.kicad_sch`. Hierarchical labels for the SPI bus, INTn, RSTn, `3V3_LOGIC`,
`GND_LOGIC` per this project's existing convention (hierarchical + sheet pin, matching
`core-compute.md`'s pattern — this subsystem doesn't cross the board-to-board header, so
no header-side considerations apply here).

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
   termination, revisit decision 4 — the "integrated, no external network" call is high
   confidence, not a datasheet-guaranteed certainty.

All 5 items require real hardware — Rev-A bring-up.

## Step 7: documentation & BOM — not done yet

Pending Step 4. Will add an Ethernet subsystem section to `hardware/datasheets/README.md`
(already added 2026-09-10, ahead of schematic capture — see that file) and write
`hardware/bom/ethernet_bom.csv` once reference designators exist on the actual schematic.

## Step 8: sign-off — not yet

Pending Steps 4-7.

## Steps

1. **Front end: W5500, crystal, support passives** — **DONE (this doc)**.
2. **Magnetics + RJ45 selection** — **DONE (this doc)** — Würth 7499010441, part number
   and connector type visually re-confirmed against reference-hardware photos; Bob-Smith
   termination resolved as "integrated, no external network" with high confidence (see
   decision 4).
3. **LED + control line assignment** — **DONE (this doc)**.
4. **Schematic capture (KiCad)** — **NEXT UP**, hands-on by Ozcan, no longer blocked.
5. **Verification checklist / ERC** — Deferred to the single end-of-project pass.
6. **Acceptance criteria** — Drafted (this doc), to confirm once Step 4 is done.
7. **Documentation & BOM** — Not started, pending Step 4.
8. **Sign-off** — Not started.

## Revision history

| Date | Change |
|---|---|
| 2026-09-10 | Doc created. Steps 1 and 3 locked from the W5500 datasheet, cross-checked against WIZnet's own published reference schematic (crystal, EXRES1, TOCAP, 1V2O, LED resistor values all independently confirmed twice). Step 2 (magnetics/RJ45) drafted but not fully locked — Würth 7499010441's datasheet appears to show integrated GDT + integrated Bob-Smith termination, which would make WIZnet's externally-added termination network redundant, but the automated datasheet read couldn't produce a clean confirmed pin diagram; flagged as an explicit open item to resolve before Step 4 wiring, not guessed either way. RSTn wired to a dedicated GPIO (decision 6) — a deliberate design choice for firmware-recoverable resets, not a datasheet requirement, open to reconsideration at roadmap step 6 if MCU pin budget is tight. |
| 2026-09-10 | **Resolved decision 4 using the reference-hardware photos**, per Ozcan's reminder that the reference teardown photos and this project's earlier component decisions are a source to check before treating something as unresolved. Found the actual magjack on the reference LTEBOARD (`photo_22`, `photo_24`): case marking reads "WE 7499010441" — an exact, direct part-number match against the physical reference part, not just a datasheet-search pick. Also checked the board's own trace layout around the jack for termination-network evidence: no discrete resistor/capacitor cluster sits next to the connector (where an external Bob-Smith network would have to be, for signal-integrity reasons, if one existed) — consistent with the datasheet's own suggestion that termination is built into the part. Combined, this raises decision 4 to "high confidence, not blocking" rather than "unresolved." Step 2 promoted to DONE; Step 4 (schematic capture) no longer blocked. Also confirmed via the same photos that the enclosure's Ethernet cutout (`photo_27`) is RJ45-shaped, distinct from the small terminal-block cutouts used elsewhere on the panel — no connector-type ambiguity for this subsystem. Added the Ethernet subsystem section to `hardware/datasheets/README.md` ahead of schematic capture (documentation-first, since the part choice is now locked). |
