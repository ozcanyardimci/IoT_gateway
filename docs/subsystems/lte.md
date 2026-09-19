# LTE Subsystem — Build Plan

**Status:** Plan drafted 2026-09-13, swept for open/uncertain items 2026-09-14. Schematic
capture (Step 4) is built out and clean in `lte.kicad_sch` (2026-09-19) — VBAT decoupling,
power control, antenna matching network, SIM interface, and PCM/PSM no-connects are all
wired, U5 (the UART level shifter) is confirmed correct, and the two small ERC-style gaps
found while checking the new master-sheet pins (D1's 4th ESD channel, two missing U4
No-Connect flags) are fixed. The LTE sheet symbol + its 6 hierarchical pins are now on the
`lteboard.kicad_sch` master sheet and verified to match `lte.kicad_sch` exactly — not yet
wired to anything else there, which needs roadmap steps 6 (pin planning) and 7
(board-to-board connector), neither started yet. Remaining open item: the modem's AC-vs-AB
revision letter (low risk, flagged in Step 4).

**Deviation from this doc, found 2026-09-18: the PWRKEY/RESET_N circuit actually built
differs from decision 3/Step 1 below.** Ozcan switched from the NMOS+external-pull-up
version to a pre-biased-transistor topology sourced from the Hardware Design doc's own
Figures 10 (PWRKEY) and 15 (RESET_N) directly — no external pull-up to `VBAT_LTE` at all
(the module has its own internal pull-up on both pins), and no debounce cap on RESET_N
(Figure 15 doesn't show one; PWRKEY's Figure 10 does). Decision 3 and Step 1's table below
are superseded by the addendum inside decision 3 and the note under Step 1 — kept in place
rather than deleted, so the reasoning trail for the earlier (also valid, just not what got
built) approach isn't lost.

## Scope

Quectel EG915U-EU LTE Cat-1 modem (with 2G fallback) on **LTEBOARD** (already the standing
architecture choice — `architecture.md`), on its own dedicated `VBAT_LTE` rail (already
provisioned in `power.md`, not re-litigated) and one of the project's 3 hardware UARTs
(also already reserved in `architecture.md`'s bus table). Not in scope: exact MCU GPIO
numbers (roadmap step 6, same deferral as everywhere else), PCB layout/trace-width
enforcement (layout stage), GNSS (this module variant doesn't support it — confirmed
below, not a scope cut).

## Design approach

Eight decisions. Two of them are real findings from reading the actual datasheets, not
routine restatements — flagged clearly where they show up.

1. **Modem: Quectel EG915U-EU — already fixed, re-confirmed against the real datasheet.**
   Read Quectel's own Hardware Design document directly 2026-09-13 (not re-derived from
   memory): VBAT_BB (pins 32/33) + VBAT_RF (pins 52/53), 3.3-4.3V range, **typical 3.8V**,
   current draw 2A (LTE only) / 3A (GSM+LTE) TX burst — matches `power.md`'s existing load
   table exactly, so no change to the current figures. Cat-1 + 2G fallback, EU band
   variant — already the intentional regional pick (the "-EU" suffix), not re-litigated.

2. **Power supply — a real margin problem, found and RESOLVED 2026-09-13.** `power.md` had
   configured this rail (U4, Würth MagI3C 171033801) with the *exact same* feedback divider
   as `3V3_LOGIC` (RFBT=402k / RFBB=137k), which computes to ~3.3V using the module's own
   datasheet formula (Vout = Vref x (RFBT/RFBB + 1), Vref=0.85V — verified directly against
   the 171033801 datasheet, and this reproduces its own 3.3V/137k reference-table entry
   almost exactly, so the formula and reference voltage are right). **That sat at the
   absolute floor of Quectel's own 3.3-4.3V spec, with zero margin — and Quectel's typical
   recommended voltage is 3.8V, not 3.3V.** The 2-3A TX burst is exactly the load that
   would sag this rail *further* through board/trace/connector resistance, right where
   there's no headroom left to give. `power.md`'s own brown-out analysis (section 10)
   checked sag on the *shared protected rail* upstream of the regulator, but never
   re-checked whether this rail's own regulated set point had margin to begin with — it
   didn't.
   **Fixed: retapped U4's RFBB from 137k to 115k (E96)**, computed the same way:
   0.85 x (402/115 + 1) = 3.82V — matching Quectel's stated typical almost exactly, and
   giving roughly even margin on both sides of the 3.3-4.3V window instead of 0V on one
   side and 1.0V on the other. **Also renamed the net from `3V3_LTE` to `VBAT_LTE`**
   (matching Quectel's own VBAT pin naming) — calling a 3.82V rail "3V3" would be actively
   misleading. The rename was fully contained (this rail had exactly one intended consumer,
   not yet wired) — see `power.md`'s 2026-09-13 revision-history entry for the exact edits
   made to `power.kicad_sch` and `ioboard.kicad_sch`, and its BOM for the corrected R6 row.
   Also from the same datasheet: VBAT decoupling should be a **100µF low-ESR (<=0.7 Ohm)**
   bulk cap plus a **100nF / 33pF / 10pF** MLCC array, all close to the VBAT pins — this is
   the specific version of the "extra output bulk capacitance at the Quectel module"
   `power.md` already anticipated generically (matching the reference board's teardown)
   without pinning down exact values; now it's pinned down. A reverse-standoff TVS on VBAT
   is also recommended — closest real standard part to Quectel's guidance is a 5.0V-standoff
   unidirectional TVS (e.g. Littelfuse/Bourns SMAJ5.0A; 4.7V isn't a common catalog value,
   5.0V is the nearest standard step at or above it). VBAT_BB/VBAT_RF trace widths (2mm /
   2.5mm minimum) are a layout-stage item, noted here so it isn't lost.

3. **Power control: PWRKEY via a GPIO-driven transistor, RESET_N as a documented
   emergency-only spare.** PWRKEY (pin 15, active-low, VBAT domain — not the 1.8V I/O
   domain) needs a low pulse of >=2s to power on, >=3s to power off (or `AT+QPOWD` for a
   clean software power-off). Quectel's own guidance: open-drain driver, never hardwire it
   permanently low. Plan: a small N-MOSFET or NPN transistor, drain/collector to PWRKEY,
   source/emitter to `GND_LOGIC`, gate/base driven from a dedicated MCU GPIO through a
   series resistor — GPIO idle high (transistor off, PWRKEY floats/pulled up by the
   module), GPIO pulses low-side-on for the required duration to pull PWRKEY low. Matches
   this project's standing preference for firmware-recoverable, GPIO-observable control
   signals (same reasoning as Ethernet's RSTn). RESET_N (pin 17, same domain, >=100ms low
   pulse) performs a baseband reset without a full power cycle — Quectel's own caution says
   to use it "only when you fail to turn off the module with AT+QPOWD or PWRKEY," so it's
   wired to its own spare GPIO via the same kind of transistor stage, but treated as an
   emergency-recovery path in firmware, not a routine control line — same "documented
   spare" pattern already used elsewhere (SPDLED/DUPLED on Ethernet, channel 2 on RS232).
   **Addendum 2026-09-14: values that were previously TBD, now pinned down from Quectel's
   own reference design (Figures 10/11 for PWRKEY, 15/16 for RESET_N — checked directly,
   not assumed).** Both circuits use the identical topology: a **4.7kOhm pull-up from the
   pin to `VBAT_LTE`** (so it idles high/inactive) in series with a **47kOhm resistor
   between the transistor's drain/collector and the pin itself** (limits current if the
   GPIO pulses while VBAT_LTE is present), plus a **10nF debounce cap** from the pin to
   `GND_LOGIC`. The transistor itself still isn't named by Quectel (their figure shows a
   generic driver block) — a generic small-signal N-MOSFET (e.g. 2N7002, SOT-23) is the
   simplest fit here: GPIO drives the gate directly, no base resistor needed (MOSFET gates
   don't draw DC current), only an optional ~100Ohm gate series resistor as ringing
   suppression, standard practice not a datasheet requirement. Two of these pull-up/series/
   debounce triplets are needed — one for PWRKEY, one for RESET_N.
   **Addendum 2026-09-18: topology actually built is different from the above, sourced
   from a cleaner pair of reference figures — Hardware Design doc's own Figure 10 (PWRKEY)
   and Figure 15 (RESET_N), read directly rather than the Reference Design doc's Fig
   10/11/15/16.** Both figures show a **pre-biased digital transistor** pattern: base ←
   4.7kOhm series resistor ← MCU GPIO pulse; a 47kOhm resistor from that same base node to
   the emitter (holds the transistor off by default — a bias resistor, not a pull-up);
   emitter → `GND_LOGIC`; collector → PWRKEY/RESET_N directly, **no resistor in the
   collector path and no external pull-up to `VBAT_LTE` at all** — the module has its own
   internal pull-up on both pins, confirmed by these figures never showing one. Figure 10
   (PWRKEY) shows a 10nF cap from the pin to GND; **Figure 15 (RESET_N) does not** — built
   with a cap on both branches anyway for consistency (harmless, still under the
   datasheet's 10nF ceiling), but only one of the two figures actually calls for it.
   Built with **Q4 (RESET_N) / Q5 (PWRKEY), generic `Q_NPN`**, **R33/R35 = 4.7kOhm**
   (base series), **R34/R36 = 47kOhm** (base-emitter), **C26 = 10nF** (PWRKEY cap; no C25
   on RESET_N, matching Figure 15). Real transistor part still open — a generic small-
   signal NPN (e.g. MMBT3904, BC847) is fine for now; Rohm's **DTC043ZEBTL** (pre-biased,
   4.7k/47k built in) is the drop-in single-part alternative if BOM count matters later.

4. **UART: MAIN_TXD/MAIN_RXD only, through a 2-channel level shifter — no hardware flow
   control.** Quectel's UART is a 1.8V I/O domain; the ESP32-S3's UART pins are 3.3V logic
   — these can't connect directly, a level shifter is mandatory, not optional. Quectel's
   own hardware design doc names TXS0108EPWR (an 8-channel part) as an example. Only 2
   lines are actually needed here (TXD/RXD) — RTS/CTS flow control is supported but not
   required, and this project has consistently kept UART interfaces minimal elsewhere
   (RS485, RS232) rather than adding flow control that a Cat-1/2G AT-command-and-moderate-
   throughput link doesn't need. Recommending a 2-channel part sized to the actual line
   count instead — TI TXB0102 — rather than carrying over an 8-channel part chosen for a
   different reference design's line count. RTS/CTS/DTR/RI all left unconnected
   (documented spare, same pattern as decision 3's RESET_N) — easy to add later if a real
   throughput/sleep-mode need shows up, without having under-provisioned the level shifter
   (TXB0102 -> TXB0104 is a pin-compatible-family upgrade path if that happens).
   **Addendum 2026-09-13: TXB0102 needs a real VCCA supply, not just signal lines — found
   via a direct re-read of the datasheet's own level-shifter reference circuit.** The module
   provides exactly this: **VDD_EXT (pin 29), 1.8V nominal, 50mA max**, meant for powering
   external circuits like this. Datasheet's own reference design feeds VDD_EXT into
   TXS0108EPWR's VCCA pin with a 2.2uF cap — same pattern applies directly to TXB0102 here
   (VCCA = VDD_EXT/1.8V, VCCB = `3V3_LOGIC`/3.3V). This was missing from the original plan;
   without it the level shifter has no low-side reference and the UART link would not work.
   **Addendum 2026-09-14: OE (pin 6) also needs a defined connection, found while writing out
   TXB0102's full pin table.** Per TI's datasheet, OE is active-high output-enable — floating
   it risks unpredictable output state, and the datasheet's own power-sequencing note says OE
   should be held low (high-Z) until VCCA/VCCB are stable, then driven high to enable. Since
   this design has no need to ever disable the level shifter in firmware, the standard fixed
   solution applies: **pull-up resistor from OE to VCCA** (same rail OE's power-up timing is
   referenced to), enabling it automatically once VCCA ramps, with no extra GPIO needed. Adds
   one resistor to the BOM — **finalized at 10kOhm** (TXB0102's OE input draws negligible
   current, so any value from ~4.7k-100k works; 10k is the standard default for an enable
   pull-up with no other constraint).

5. **SIM interface: direct-wired micro-SIM holder, no hot-plug detection.** USIM1_DATA /
   USIM1_CLK / USIM1_RST / USIM1_VDD / USIM1_GND wire straight to a micro-SIM push-push
   holder's contacts (J4, **JAE SF53S006VCBR2000** — confirmed real part, imported and placed
   2026-09-15) — matches the reference-hardware LTEBOARD's own SIM holder (visible in
   `reference-photos/2026-09-08-hardware-assembly-photos/photo_23`, `photo_29`, `photo_30`).
   Voltage (1.8V or 3.0V) is auto-detected by the module, no external logic needed. Per
   datasheet: bypass cap <=1uF at VDD close to the connector, a TVS array (<15pF parasitic
   capacitance, to avoid loading the DATA/CLK lines) for ESD on the exposed contacts, and an
   optional pull-up on DATA for noise immunity (adding it — cheap, datasheet-recommended).
   USIM1_DET (hot-plug/card-present sense) left as a documented spare, unused — this is an
   assembled-once industrial gateway, not a field-swappable-SIM consumer device, so
   hot-plug detection doesn't earn its keep here.
   **Addendum 2026-09-14: both previously-open parts on this line, now pinned down —
   corrected once more after finding the Hardware Design doc's own SIM reference circuit
   (Figures 22/23) directly, which is more precise than the Reference Design doc used
   initially.** (1) Pull-up: **15kOhm, on USIM1_DATA only — not CLK** (Figure 22/23
   explicitly show just one pull-up, on DATA; the earlier addendum guessed both lines
   before this more detailed figure was checked). It returns to **USIM1_VDD** (this
   interface's own auto-detected 1.8V/3.0V supply), not an external logic rail — matches
   Quectel's own description of it as an anti-jamming measure local to the SIM supply
   domain. USIM1_VDD itself also gets a **100nF bypass cap to `GND_LOGIC`** per the same
   figures (satisfies the "<=1uF" ceiling from the general datasheet guidance with margin).
   (2) TVS/ESD/filter part: Quectel's own docs never name a manufacturer part for this
   (checked directly — their schematic just labels it "U0701/U0702," and separately shows
   discrete 33pF filter caps on DATA/CLK/RST). Found a real, purpose-built part instead via
   Nexperia's own SIM-protection app note (AN10914): **Nexperia IP4264CZ8-10** (QFN,
   3-channel — RST/CLK/DATA, 10-12pF typ/max per channel, described by Nexperia as an
   "integrated (U)SIM card passive filter array with ESD protection") — comfortably meets
   the <15pF requirement with margin, and being an integrated filter+ESD part, replaces
   the need for Quectel's separate discrete 33pF caps on those 3 lines (one part instead of
   three caps plus a separate TVS array). VDD isn't one of its 3 channels; its own 100nF
   bypass cap is the protection there, per the figures above.
   **Addendum 2026-09-15: IP4264CZ8-10 swapped out — no SnapEDA/Ultra Librarian symbol or
   footprint found (checked by Ozcan directly), and its QFN package with no SnapEDA listing
   also would've meant hand-drawing the symbol/footprint from scratch, on a part with no
   confirmed Turkey-side distributor stock either.** Replaced with **STMicroelectronics
   ESDA6V1BC6** — confirmed on SnapEDA (symbol + footprint + 3D model, part number matches
   exactly), 6.1V standoff quad bidirectional TVS array in **SOT23-6L** (6 gull-wing leads —
   easier to hand-solder than the QFN part it replaces), and confirmed in stock directly
   through Mouser's own Turkey storefront (mouser.com.tr lists it under this exact part
   number) — a real, checked local-purchase path, not just "Mouser ships internationally."
   Trade-off, stated plainly: this is a **generic ESD array, not an integrated filter+ESD
   part** like the Nexperia one was — its datasheet gives ~20pF/channel, which is ESD
   protection capacitance only, with no built-in EMI filtering. That means **Quectel's
   original discrete 33pF filter caps on DATA/CLK/RST go back into the BOM** (3 caps,
   removed from decision 5's plan when the Nexperia part looked like it could absorb that
   function) — this is not a downgrade, it's reverting to exactly what Quectel's own
   reference schematic does (a generic ESD block, U0701/U0702, plus separate discrete filter
   caps); the one-chip "filter+ESD combined" version was a nice-to-have simplification, not
   a requirement, and it's fine to lose it in exchange for a part that's actually sourceable
   and solderable. 4 channels are available on ESDA6V1BC6; only 3 are needed (RST/CLK/DATA),
   so one channel is left unused — exact pin mapping comes up when we reach that pin in the
   walkthrough.

6. **Status pins: STATUS and NET_STATUS left open, not wired to the MCU.** Both are
   open-drain, 1.8V-domain outputs. Reading them directly into a 3.3V-logic MCU GPIO is the
   kind of thing that looks fine on paper but is actually unreliable — a 1.8V pull-up won't
   reliably clear a 3.3V-logic input's high threshold, so it would need its own
   level-shifting or comparator stage just to read two status pins. `status-indication.md`
   already gives LTE its own status LED (D_LTE) driven through the PCA9535 I2C expander
   under firmware control (AT-command polling — `AT+CREG`/`AT+QNETDEVSTATUS`-style queries,
   same pattern as the HEARTBEAT and FAULT indicators, which are also firmware-aggregated,
   not hardware-pin-driven). That already covers what these pins would tell a technician,
   without the extra level-shifting complexity. Quectel's own datasheet explicitly allows
   leaving them open if unused.

7. **Antenna: same pattern as WiFi, independently confirmed on the same reference board.**
   ANT_MAIN (pin 60, 50 Ohm) needs an external antenna — Quectel's own doc recommends SMA.
   Confirmed against the reference-hardware photos (`photo_21`, the full-board shot) that
   this project's actual antenna interface style repeats here exactly: module's own antenna
   feed -> short coax pigtail -> PCB-edge-mount SMA female jack, no PCB RF trace — the
   identical topology `wifi.md` established for the ESP32-S3's antenna, now confirmed a
   second time on the same board for a second radio. On the schematic: SMA jack symbol,
   shield to `EARTH` (same convention as WiFi's J2, Ethernet's magjack shield, RS485's GDT),
   RF pin No Connect (fed by the physical cable, not a PCB net). ANT_BT/WIFI_SCAN (pin 56,
   the module's secondary receive-only antenna, shared with an unused onboard Bluetooth/
   WiFi-scan radio) is left NC — redundant given this project already has its own,
   independent ESP32-S3 WiFi radio; no reason to bring out a second, receive-only, can't-
   transmit antenna path for a feature this design doesn't use. Confirmed from the
   datasheet: **no GNSS on this module variant** — not a scope cut, the hardware simply
   doesn't have it.
   **Addendum 2026-09-14: off-board antenna part, previously unselected, now has a concrete
   candidate.** TE Connectivity/Linx **ANT-LTE-MON-SMA-L** — wideband (distributor listings
   put it at ~617MHz-3.8GHz), SMA male, tilt/swivel mount, stocked at DigiKey/Mouser. That
   range covers every EU cellular LTE band this module uses with margin (I could not pull
   Linx's own independent PDF datasheet to double-verify the band figure directly — only
   distributor listing text — so treat the exact number as reasonably but not fully
   verified; still a sound pick either way, since "full-range cellular whip/stubby
   antenna" is the standard, low-risk choice recommended by every source checked, including
   Digi's own LTE Cat-1 antenna guide, which deliberately declines to name one "approved"
   part and instead says any wideband SMA antenna from a reputable supplier is fine).
   **Also re-confirmed the no-GNSS finding directly**, prompted by a DigiKey listing for the
   exact ordering code in use (`EG915UEUAC-N05-SNNSA`) that claims "GNSS included" — checked
   Quectel's Hardware Design doc and its separate Module Specification doc a second time,
   independently; neither lists any GNSS pin or capability for the EU variant. Treating the
   DigiKey text as a templated/incorrect distributor description (a known issue with
   auto-generated listings) and this finding as settled: **no GNSS**, no antenna path needed
   for it.

8. **Power/ground: `VBAT_LTE` / `GND_LOGIC`, already budgeted — documentation gap upstream,
   RESOLVED 2026-09-13.** `power.md` already sizes and provisions this rail (decision 2
   covers its voltage-margin fix). One thing this subsystem surfaced that wasn't really an
   LTE-subsystem problem: `architecture.md`'s board-to-board header section listed header
   power lines generically as "power (3.3V, 5V, GND)," without calling out `VBAT_LTE` as
   its own separate header allocation from `3V3_LOGIC`. Since the entire reason `VBAT_LTE`
   is a separate rail is to keep its 2-3A transient from coupling into anything else
   (`power.md`'s own reasoning), it needs its own dedicated header pin(s) — sharing
   copper with `3V3_LOGIC` on the connector would partly undo that separation, and 2-3A
   through a single small 0.1" header pin is worth checking against that pin's own current
   rating (may need 2+ parallel pins — still a real open item for roadmap step 7's actual
   pin-out work). **Fixed the documentation gap 2026-09-13**: `architecture.md`'s header
   section now lists `VBAT_LTE` as its own line, separate from 3.3V-LOGIC, with the
   reasoning above attached so it isn't lost between now and step 7.

## Step 1 results: power & control (drafted 2026-09-13)

| Parameter | Value | Source |
|---|---|---|
| VBAT_BB / VBAT_RF | 3.3-4.3V (typ 3.8V), 2A (LTE) / 3A (GSM+LTE) TX burst | Quectel EG915U-EU Hardware Design v1.1 |
| VBAT decoupling | 100uF low-ESR (<=0.7 Ohm) + 100nF/33pF/10pF MLCC array, close to pins | Same |
| VBAT protection | TVS, standoff >= module's max VBAT; closest real part SMAJ5.0A | Same (guidance); Littelfuse/Bourns SMAJ series (representative) |
| `VBAT_LTE` rail set point | ~3.82V (115k/402k divider, retapped 2026-09-13 from 137k/~3.3V), see decision 2 | `power.md` + Würth 171033801 datasheet (formula/Vref) |
| PWRKEY | Pin 15, active low, VBAT domain, >=2s on / >=3s off pulse, via GPIO+transistor | Quectel datasheet |
| RESET_N | Pin 17, active low, VBAT domain, >=100ms pulse, emergency-only spare | Quectel datasheet |
| PWRKEY/RESET_N pull-up | ~~4.7kOhm, pin to `VBAT_LTE`~~ **Superseded 2026-09-18 — see below** | ~~Quectel reference design Fig 10/11, 15/16~~ |
| PWRKEY/RESET_N series R | ~~47kOhm, transistor to pin~~ **Superseded 2026-09-18 — see below** | ~~Same~~ |
| PWRKEY/RESET_N debounce cap | ~~10nF, pin to `GND_LOGIC`~~ **Superseded 2026-09-18 — see below** | ~~Same~~ |
| PWRKEY/RESET_N transistor | ~~Generic small-signal NMOS, e.g. 2N7002 (SOT-23)~~ **Superseded 2026-09-18 — see below** | ~~Decision 3 addendum~~ |
| **PWRKEY/RESET_N — as built (2026-09-18)** | Pre-biased-transistor topology: **R33/R35 = 4.7kOhm** base series, **R34/R36 = 47kOhm** base-emitter, **Q4/Q5 = generic `Q_NPN`**, **C26 = 10nF** on PWRKEY only (no cap on RESET_N). **No external pull-up to `VBAT_LTE`** — relies on the module's internal pull-up on both pins. | Quectel Hardware Design doc Fig 10 (PWRKEY) / Fig 15 (RESET_N), read directly — decision 3 addendum 2026-09-18 |

## Step 2 results: UART & SIM (drafted 2026-09-13)

| Parameter | Value | Source |
|---|---|---|
| UART lines used | MAIN_TXD (35), MAIN_RXD (34) only | Quectel datasheet + decision 4 |
| Level shifter | TI TXB0102 (2-channel), 1.8V <-> 3.3V | Quectel datasheet (recommends the concept; part sized to actual line count here) |
| UART lines left spare | MAIN_RTS/CTS/DTR/RI (36/37/30/39) | Decision 4 |
| Level shifter VCCA supply | VDD_EXT (pin 29), 1.8V nom, 50mA max, +2.2uF cap at the pin | Quectel datasheet (addendum 2026-09-13) |
| Level shifter VCCB supply | `3V3_LOGIC` | Project convention |
| Level shifter OE | ~~Pull-up resistor to VCCA, 10kOhm (finalized) — always-enabled~~ **Superseded 2026-09-19 — as built, it's a divider: R22 (10kOhm) from `VDD_EXT` down to R31 (120kOhm) to `GND_LOGIC`, OE ≈1.66V, comfortably above threshold — see Step 4** | TI TXB0102 datasheet (addendum 2026-09-14; corrected 2026-09-19 against the live schematic) |
| SIM connector | J4, **JAE SF53S006VCBR2000**, 6-pin micro-SIM push-push holder | Matches reference-hardware photos; real part confirmed 2026-09-15 |
| SIM connector pinout | ISO 7816 standard contacts: C1=VCC, C2=RST, C3=CLK, C5=GND, C6=VPP, C7=I/O (C4/C8 not present on this 6-contact part) | Standard, confirmed against the placed symbol's own pin labels |
| SIM connector C6 (VPP) | **No-Connect** — the modem's SIM interface has no VPP pin at all (modern cellular modems don't implement it) | Found 2026-09-15 while mapping the real connector's pins |
| SIM VDD bypass | **100nF**, USIM1_VDD to `GND_LOGIC` (finalized) | Quectel Hardware Design Fig 22/23 (addendum 2026-09-14) |
| SIM DATA pull-up | **15kOhm, DATA only (not CLK)**, to USIM1_VDD (finalized) | Quectel Hardware Design Fig 22/23 (addendum 2026-09-14) |
| SIM ESD/filter array | **STMicroelectronics ESDA6V1BC6** (SOT23-6L, quad TVS, 4-ch, 3 used for RST/CLK/DATA, ~20pF/ch) | SnapEDA + Mouser Türkiye confirmed (addendum 2026-09-15, replaces IP4264CZ8-10) |
| SIM filter caps (RST/CLK/DATA) | **33pF** discrete, one per line (reinstated — ESDA6V1BC6 is ESD-only, no built-in filter) | Quectel Hardware Design reference schematic (addendum 2026-09-15) |
| SIM DET | Left unused (documented spare) | Decision 5 |

## Step 3 results: antenna (drafted 2026-09-13)

| Parameter | Value | Source |
|---|---|---|
| ANT_MAIN | Pin 60, 50 Ohm, coax pigtail -> PCB SMA jack -> external antenna, no PCB RF trace | Quectel datasheet + reference-hardware photos |
| Board connector shield | `EARTH` (same convention as WiFi/Ethernet/RS485) | Project convention |
| ANT_BT/WIFI_SCAN | Pin 56, NC — redundant with the project's own WiFi radio | Decision 7 |
| GNSS | Not supported on this module variant — re-confirmed 2026-09-14 against a conflicting distributor listing | Quectel Hardware Design + Module Specification docs (2 independent checks) |
| Off-board antenna | TE Connectivity/Linx ANT-LTE-MON-SMA-L, ~617MHz-3.8GHz, SMA, tilt/swivel | DigiKey/Mouser listings (addendum 2026-09-14; see caveat in decision 7) |

## Step 4: schematic capture — sheet done and clean; master-sheet wiring pending on steps 6/7

Given the component count (level shifter, PWRKEY/RESET transistor stages, SIM holder,
antenna jack, plus the VBAT decoupling/TVS network), this gets its own hierarchical sheet —
`hardware/kicad/lteboard/lteboard/lte.kicad_sch` — rather than piggybacking on
`core-compute.kicad_sch` the way WiFi's single connector did. Hierarchical labels crossing
into this sheet: `VBAT_LTE` (input), `GND_LOGIC` (global, project-wide), `3V3_LOGIC` (input,
now also needed for the level shifter's VCCB), `LTE_TXD`/`LTE_RXD` (post-level-shift, 3.3V
domain — input/output matching the MCU's perspective, same `ETH_`-style prefix convention),
`LTE_PWRKEY`/`LTE_RESET` (input — MCU drives these). VDD_EXT and MAIN_TXD/MAIN_RXD (pre-shift,
1.8V side) stay as local nets within this sheet, between the modem and the level shifter —
they don't cross to another sheet.

**Status as of 2026-09-18, checked directly against the live `lte.kicad_sch`:** VBAT
decoupling (D3/C_bulk3/C_bulk4/C19-C24), the PWRKEY/RESET_N transistor stage (Q4/Q5 +
R33-R36 + C26), the ANT_MAIN matching network (J3/D4/R32/C0603/CO604), SIM interface
(J4/D1/R25/R28/R29/R30/C13/C16-C18), and PCM/PSM/status no-connects are all present and
wired. (R25/R29/R30 corrected here 2026-09-19 — they're SIM RST/DATA/CLK line links, not
antenna network components; see below.)
U4 (modem) and U5 (TXB0102 level shifter) are both placed.

**U5 (level shifter) wiring — checked twice, confirmed correct as built (2026-09-19).**
A first pass on 2026-09-18 (tracing pin coordinates by hand through the raw `.kicad_sch`
text) wrongly concluded VCCB/GND and VCCA/OE were swapped. That was a math error on my
side, not a schematic problem: converting a symbol's pin position from the library's own
coordinate frame (+Y up) to the sheet's frame (+Y down) requires negating Y even when the
symbol has no explicit mirror — mirroring is a second, additional flip on top of that base
one. The first pass only applied the mirror's flip and missed the base one, which
swapped exactly the two pin-pairs that sit as mirror images of each other (VCCA↔OE,
GND↔VCCB) — precisely the false "defect" reported. Redone with the correct transform and
checked against a screenshot of the actual symbol: **VCCB = `3V3_LOGIC`** (via C11, 100nF),
**GND = `GND_LOGIC`**, **VCCA = `VDD_EXT`** (via C_BYPASS1, 2.2uF) — all correct, matching
decision 4. **OE** is fed by a resistor divider from `VDD_EXT` down to `GND_LOGIC` (R22 =
10kOhm on top, R31 = 120kOhm on the bottom, giving OE ≈1.66V) rather than the bare pull-up
this doc originally specified — that's fine functionally (comfortably clears the TXB0102's
input-high threshold referenced to VCCA/1.8V) and arguably more deliberate than a plain
pull-up, just not what Step 2's table below currently says; table updated to match.
A1/A2 ↔ B1/B2 channel pairing was also checked and is correct: A1(pin5)=MAIN_TXD pairs with
B1(pin8)=LTE_RXD, A2(pin4)=MAIN_RXD pairs with B2(pin1)=LTE_TXD — standard TXD-to-RXD
crossover from the MCU's perspective.

No fix needed on U5. This item is closed.

**Two smaller housekeeping items found in the same pass, non-blocking:**
1. The antenna matching network's two "NM" (not-mounted) shunt caps carry non-standard
   reference designators — `C0603` and `CO604` (note: the second uses the letter O, not
   the digit 0 — likely copied by hand from the Quectel reference figure's own labels
   rather than assigned by KiCad's annotation). Worth a re-annotate pass (Tools → Annotate
   Schematic) to give them normal sequential designators along with everything else.
2. `C_BYPASS1` and `C11` each carry a second, stale instance-path entry under a project
   named `lte` alongside the current `lteboard` project — leftover metadata from whenever
   this sheet was copied between projects. Harmless (KiCad only uses the current project's
   path) but will also get cleaned up by the same re-annotate pass.

**Master-sheet integration, checked 2026-09-19.** Ozcan added the LTE sheet symbol and its
6 hierarchical pins to `lteboard.kicad_sch` (the master sheet): `3V3_LOGIC`, `LTE_PWRKEY`,
`LTE_RESET`, `VBAT_LTE`, `LTE_RXD`, `LTE_TXD`. Checked this the same way as the U5
retraction — full netlist reconstruction from the raw `.kicad_sch` files (parsing
`lib_symbols` pin geometry, every placed symbol's mirror/rotation, every wire, junction, and
label, unioning them into nets), not eyeballing coordinates by hand, precisely because that
manual approach is what caused the U5 mistake. Result: **all 6 pins match exactly** — same
name and same electrical shape (input/output) as the corresponding hierarchical label inside
`lte.kicad_sch`, and each one lands on the correct internal net (`LTE_TXD`→U5 pin1/B2,
`LTE_RXD`→U5 pin8/B1, `3V3_LOGIC`→U5 pin7/VCCB, `LTE_PWRKEY`→R35 base resistor, `LTE_RESET`→
R33 base resistor, `VBAT_LTE`→the shared VBAT_BB/VBAT_RF decoupling bank feeding U4). No
mismatch, no dangling boundary pin.

The same reconstruction also resolved one previously-flagged open item and turned up two new
ones, unrelated to the master-sheet pins themselves but worth catching before sign-off:

- **Resolved:** R25/R29/R30's role (flagged "unconfirmed" in Step 7 below). They're 0-ohm
  links in series with the SIM interface's RST/DATA/CLK lines respectively, sitting between
  connector J4 and the D1 ESD/filter-cap node on each line (R25=RST, R29=DATA, R30=CLK) —
  not mystery components, just zero-ohm placeholders on each signal line. BOM updated.
- **Found and fixed — D1's 4th ESD channel was unwired.** D1 (ESDA6V1BC6, quad TVS array)
  has one channel per SIM signal. Three were correctly wired: pin1→`USIM1_DATA`,
  pin3→`USIM1_RST`, pin6→`USIM1_CLK`. The 4th, pin4, carried a label reading `USIM1_VDD` but
  no actual wire to the real net (U4 pin43 + C13 + R28 + J4 pin C1) — a planned-but-never-
  finished connection. **Fixed same day:** pin4 is now wired into the `USIM1_VDD` net,
  giving the SIM supply line real 4-channel ESD protection. Re-verified against a fresh pull
  of the file.
- **Found and fixed — two U4 pins were missing their No-Connect flag.** `PSM_EINT` (pin 96)
  and `PSM_IND` (pin 1) sit in the same unused-pin block as `GRFC1`/`GRFC2`/the PCM lines —
  every other pin in that block had a No-Connect marker, these two didn't (missed during the
  2026-09-18 "PCM/PSM no-connects" pass). **Fixed same day**, re-verified.

**On the master sheet, the 6 new pins aren't wired to anything yet** — expected at this
stage, not a defect (matches "I added the labels, remaining steps next"). Two different
situations once wiring starts:
- `3V3_LOGIC` and `VBAT_LTE` can be wired now: `3V3_LOGIC` just needs to join the same bus
  already tying core-compute/ethernet/status_indication's `3V3_LOGIC` pins together on this
  sheet. `VBAT_LTE`, though, doesn't actually originate anywhere on this master sheet or
  anywhere else in the `lteboard` project — the real `VBAT_LTE` rail lives in `power.md` /
  `power.kicad_sch`, which is on the *other* board (`ioboard`, a separate KiCad project).
  Neither board's master sheet currently has a board-to-board connector symbol drawn — this
  matches `roadmap.md` step 7's explicit "board-to-board interconnect test milestone," which
  is marked **NOT STARTED**, so this isn't something specific to LTE; every cross-board rail
  is in the same state.
- `LTE_PWRKEY`, `LTE_RESET`, `LTE_TXD`, `LTE_RXD` need real MCU-side GPIO/UART pins to wire
  to, and `core-compute.kicad_sch`'s own sheet symbol currently only exposes `3V3_LOGIC` and
  the I2C bus (`I2C_SCL`/`I2C_SDA`) — no GPIOs, no UART. Assigning those is `roadmap.md` step
  6, "fine-grained resource planning," also **NOT STARTED**. So these four can't be finished
  until that step happens — not a defect in what's built so far, just the dependency order.

**Symbol sourcing, checked 2026-09-14, SIM holder resolved 2026-09-15:** neither the modem
nor the level shifter has a default-KiCad-library symbol/footprint — confirmed both are
available for import instead. Quectel **EG915UEUAC-N05-SNNSA** (the exact ordering code
being used) has a symbol, footprint, and 3D model on SnapEDA under that exact part number.
TI **TXB0102DCUR** likewise has one on both SnapEDA and Ultra Librarian. The micro-SIM
holder — **JAE SF53S006VCBR2000** — has now been imported and placed as J4; its symbol
shows contacts C1/C2/C3/C5/C6/C7 (standard ISO 7816 numbering, C4/C8 not present on this
6-contact part), confirming the earlier uncertainty about the paired symbol name is resolved
under `Connector_Card` before schematic capture.

**Remaining genuinely open item, not resolved by research:** the ordering code's hardware
revision letter — `EG915UEUAC` uses revision "AC," while an "AB" revision of the same EU SKU
is also in distribution. No revision-delta changelog was found from Quectel describing what
changed between them. Low risk (same family, same region variant, same datasheet should
apply), but flagging rather than asserting it's identical — if you want certainty, this is a
"ask Quectel/a distributor rep" question, not something further web research is likely to
resolve.

## Step 5: verification checklist / ERC — not run yet

Deferred to the single end-of-project ERC pass, same policy as every subsystem since
RS232. One LTE-specific thing to double check there: the level shifter's direction pins
(if the chosen part needs an explicit direction control rather than auto-sensing — TXB0102
is auto-sensing/no direction pin needed, so this should be a non-issue, but worth
confirming once the real part is placed).

## Step 6: acceptance criteria — draft

1. `VBAT_LTE` holds within Quectel's 3.3-4.3V range under the full 2-3A TX transient,
   including board/trace/connector IR drop — this is exactly what decision 2's margin fix
   is for; re-verify in the ratio actually built, whichever way that decision is resolved.
2. Module powers on reliably via the PWRKEY transistor stage (>=2s low pulse) and powers
   off cleanly via `AT+QPOWD` — bench item.
3. UART communicates at the configured baud rate through the level shifter with no
   corruption — bench item. **Firmware TODO, not hardware:** since RTS/CTS are left
   unwired (decision 4), hardware flow control must be disabled in the modem's own UART
   config (Quectel AT command, e.g. `AT+IFC=0,0`) — otherwise the module may wait for a
   CTS assertion that will never come. Tracked here so it isn't forgotten at firmware time.
4. SIM is detected and registers on the EU band network — bench item, needs a real SIM.
5. LTE associates and holds a data session at expected signal quality with the chosen
   antenna — bench item.
6. D_LTE status LED (via firmware AT-command polling) correctly reflects registration/data
   state — bench item, firmware-side.

All of 2-6 need real hardware — Rev-A bring-up, same pattern as every other subsystem.

## Step 7: documentation & BOM — done 2026-09-19, from real reference designators

LTE subsystem section added to `hardware/datasheets/README.md` and `hardware/bom/lte_bom.csv`
written, pulled directly from the live schematic's actual reference designators — not
placeholders. Originally marked **draft** (2026-09-18) pending a re-check of U5, which at
the time was wrongly believed to have a wiring defect (see Step 4 — that was a coordinate-
math error on my end, not a real problem); with that resolved 2026-09-19, both files and
the BOM are confirmed accurate and this step is done, not just drafted. R31's role was
resolved 2026-09-19 (low side of U5's OE divider), and R25/R29/R30's role was also resolved
2026-09-19 while re-checking the master-sheet integration — 0-ohm links in series with the
SIM RST/DATA/CLK lines, between J4 and D1 (see Step 4). No BOM rows remain flagged
unconfirmed.

## Step 8: sign-off — not yet

Pending Steps 5-6 (both need real hardware, blocked until Rev-A exists) and the master-sheet
wiring noted in Step 4 (blocked on roadmap steps 6/7, project-wide, not LTE-specific). The
two schematic-level gaps found 2026-09-19 (D1's 4th ESD channel, U4's two missing No-Connect
flags) are fixed and re-verified — nothing outstanding at the `lte.kicad_sch` sheet level.
Last subsystem — sign-off here closes out the entire subsystem-by-subsystem build phase
(roadmap step 4), once the master-sheet wiring and bench items are done.

## Revision history

| Date | Change |
|---|---|
| 2026-09-19 | **D1's 4th ESD channel wired in, and U4's two missing No-Connect flags added — both fixed in KiCad and re-verified.** Ozcan fixed the two gaps flagged in the master-sheet-integration check (below) directly in `lte.kicad_sch`. Re-ran the same netlist reconstruction against a fresh pull of the file (caught mid-check that the file had changed size/mtime since the first pass — pulled again rather than trusting the stale copy): D1 pin4 now lands on the real `USIM1_VDD` net (with C13/R28/J4 pin C1/U4 pin43), and both `PSM_EINT`/`PSM_IND` now carry No-Connect markers. Also re-swept every other pin in the sheet for the same "unlabeled, unconnected, not-NC" pattern — nothing else turned up. Schematic-level Step 4 has no known open items now. Updated Step 4's heading and Step 8 in this doc, and `roadmap.md`'s LTE line (schematic capture + BOM now DONE, master-sheet wiring noted as pending on roadmap steps 6/7). |
| 2026-09-19 | **Master-sheet integration checked, using a full netlist reconstruction (not hand-traced coordinates, learning from the U5 mistake above).** Ozcan added the LTE sheet symbol + 6 hierarchical pins to `lteboard.kicad_sch`; verified all 6 match `lte.kicad_sch`'s hierarchical labels exactly (name and direction) and land on the correct internal nets — no mismatch. Same pass resolved R25/R29/R30's role (0-ohm links in series with SIM RST/DATA/CLK, between J4 and D1) and found two new, previously-undocumented open items: D1's 4th ESD channel (pin 4, labeled `USIM1_VDD`) isn't actually wired to that net — just a disconnected label; and U4's `PSM_EINT`/`PSM_IND` pins are missing the No-Connect flag every neighboring unused pin has. Also confirmed the 6 new pins aren't wired to anything yet on the master sheet, which is expected, not a defect: `3V3_LOGIC`/`VBAT_LTE` need the board-to-board connector (roadmap step 7, not started, affects every subsystem's cross-board rails, not LTE-specific) and `LTE_PWRKEY`/`LTE_RESET`/`LTE_TXD`/`LTE_RXD` need real MCU pins that don't exist yet on `core-compute.kicad_sch` (roadmap step 6, not started). Updated Step 4, Step 7, Step 8, and `hardware/bom/lte_bom.csv`'s R25/R29/R30 rows. |
| 2026-09-19 | **Retracted the 2026-09-18 "U5 wiring defect" finding — it was wrong.** Ozcan pushed back twice (first "maybe it mirrored to x exen", then a screenshot of the actual circuit) after the 2026-09-18 entry below claimed VCCB/GND and VCCA/OE were swapped on U5. Re-derived the pin math from scratch: converting a KiCad symbol's pin position from the library's own coordinate frame (+Y up) to the sheet's frame (+Y down) requires negating Y even with no mirror applied at all — mirroring is a second, separate flip on top of that. The 2026-09-18 check applied only the mirror's flip and missed the base one, which happened to swap exactly the two pin-pairs (VCCA↔OE, GND↔VCCB) reported as "swapped" — a self-consistent-looking but entirely wrong result. Redone with the correct transform and checked against Ozcan's screenshot: all 8 of U5's pins land exactly where decision 4 specifies. **There was never a defect.** Corrected the Status block, Step 2's OE row, Step 4, and Step 7 in this doc, plus the matching false claims in `hardware/datasheets/README.md` and `hardware/bom/lte_bom.csv`. Also resolved R31's previously-unidentified role (OE divider low side) while re-checking U5. |
| 2026-09-18 | **Live-file check of `lte.kicad_sch` against this plan.** Confirmed most of Step 4 (schematic capture) is built: VBAT decoupling, PWRKEY/RESET_N, antenna matching network, SIM interface, PCM/PSM no-connects. Found and documented that the as-built PWRKEY/RESET_N circuit uses a pre-biased-transistor topology (Q4/Q5, R33-R36, C26) from the Hardware Design doc's Fig 10/15 directly, not the NMOS+external-pull-up version this doc originally specified (decision 3, Step 1 table both annotated, old approach kept for the reasoning trail rather than deleted). **Found a real wiring defect on U5 (level shifter):** VCCB/GND swapped (3.3V logic supply going to the chip's GND pin, ground going to its VCCB pin), and VCCA/OE effectively swapped (OE hard-wired to VDD_EXT, VCCA only reachable through a 10k resistor with a dangling stub beyond it, VCCA's intended 2.2uF bypass cap actually sitting on the OE/VDD_EXT node instead). This blocks Step 4 sign-off until fixed in KiCad. Also flagged two non-blocking annotation issues (`C0603`/`CO604` non-standard reference designators, stale dual-project instance metadata on `C_BYPASS1`/`C11`) for a future re-annotate pass. Added Step 7 draft: LTE section in `hardware/datasheets/README.md` and `hardware/bom/lte_bom.csv`, built from the schematic's real reference designators. |
| 2026-09-13 | Doc created. Modem's own datasheet read directly (not re-derived from memory) for VBAT/current figures (confirmed against `power.md`'s existing numbers — no change needed there), PWRKEY/RESET_N behavior, UART pin list and voltage domain, SIM interface requirements, status pin behavior, and antenna/GNSS capability. Antenna path (coax pigtail -> PCB SMA jack, no RF trace) confirmed against the reference-hardware photos, mirroring the exact pattern already established in `wifi.md`. Status pins (STATUS/NET_STATUS) deliberately left unwired — LTE status is already covered by the existing D_LTE LED via firmware AT-command polling (`status-indication.md`), avoiding an unreliable direct 1.8V-into-3.3V-GPIO read. |
| 2026-09-15 | **SIM ESD/filter part swapped: Nexperia IP4264CZ8-10 -> STMicroelectronics ESDA6V1BC6.** Prompted by two things: Ozcan checking directly and finding no SnapEDA/Ultra Librarian listing for the Nexperia part, and a direct challenge on whether the original recommendation had been checked against the reference-hardware photos (it hadn't — checked afterward, photos confirm the SIM holder's placement but aren't sharp enough to read any nearby IC's markings, so the original pick was datasheet/app-note-derived, not photo-verified, and that distinction should have been stated clearly the first time). New part: SnapEDA-confirmed, SOT23-6L (hand-solderable), and confirmed in stock on Mouser's Turkey storefront directly. Since it's ESD-only (no integrated filter), Quectel's original discrete 33pF caps on DATA/CLK/RST go back into the BOM — three extra passives, but this matches Quectel's own reference schematic exactly, so it's a reversion to a proven topology, not a compromise. Decision 5 and Step 2's table updated. |
| 2026-09-14 | **Full sweep of every open/TBD item in this doc, at Ozcan's request.** Resolved with concrete values/parts: PWRKEY/RESET_N pull-up (4.7k)/series (47k)/debounce cap (10nF), pulled directly from Quectel's own reference-design figures (previously generic); OE pull-up finalized at 10k; SIM DATA pull-up finalized at 15k, to USIM1_VDD (Quectel's own reference figures — corrected same day from an initial guess that both DATA and CLK got pull-ups); SIM VDD bypass finalized at 100nF; SIM ESD/filter array named — Nexperia IP4264CZ8-10, found via Nexperia's own SIM-protection app note (Quectel's docs never name one), which also replaces the separate discrete 33pF filter caps Quectel's figures show; antenna given a concrete candidate — Linx ANT-LTE-MON-SMA-L (wideband SMA); GNSS re-confirmed no-support after a conflicting DigiKey listing surfaced, checked independently against two Quectel primary documents; confirmed the modem and level shifter both have importable SnapEDA/Ultra Librarian KiCad symbols; added a firmware TODO (disable hardware flow control, `AT+IFC=0,0`) so it isn't lost. One item remains genuinely open, not resolvable by further research: whether hardware revision "AC" vs "AB" of this SKU differs in any way — low risk, flagged in Step 4. |
| 2026-09-14 | **Level shifter OE pin gap found and fixed.** Working out TXB0102's complete pin-by-pin connection table (in response to a wiring-reference request) surfaced that OE (pin 6, active-high output-enable) had no defined connection — left floating it risks unpredictable output state. Fixed: pull-up resistor from OE to VCCA, so it self-enables once VCCA (VDD_EXT) ramps, no GPIO needed. Added to decision 4 and Step 2's table; one resistor added to the BOM. |
| 2026-09-13 | **Level shifter power gap found and fixed.** Re-reading the datasheet while compiling the wiring reference list surfaced that TXB0102 needs a real VCCA supply, not just its two signal lines. The module has a purpose-built pin for exactly this — VDD_EXT (pin 29, 1.8V nom, 50mA max, +2.2uF cap) — matching Quectel's own reference circuit for their example level shifter. Added to decision 4, Step 2's table, and Step 4's sheet description; `3V3_LOGIC` also now listed as a hierarchical input to `lte.kicad_sch` (feeds VCCB). |
| 2026-09-13 | **Resolved both open findings.** (1) The rail's ~3.3V regulator set point sat at the exact floor of Quectel's 3.3-4.3V spec with zero margin against the 2-3A TX transient, below Quectel's own 3.8V typical — retapped U4's RFBB (137k -> 115k) to ~3.82V, computed from the 171033801's own Vout=Vref×(RFBT/RFBB+1) formula, and renamed the net `3V3_LTE` -> `VBAT_LTE` since a 3.82V rail called "3V3" would be misleading; contained rename (one intended consumer, not yet wired) — see `power.md`'s matching revision-history entry for the exact `power.kicad_sch`/`ioboard.kicad_sch` edits. (2) `architecture.md`'s board-to-board header line now lists `VBAT_LTE` separately from `3V3_LOGIC`, with the reasoning attached, ahead of roadmap step 7's real pin-out work. |
