# Status Indication Subsystem — Build Plan

**Status:** Complete. All 8 plan steps closed 2026-09-07, including the `power.md` LED
budget item this doc originally carried forward (closed 2026-09-10). Schematic capture
complete on both sheets, cross-sheet link to core-compute verified. ERC deferred to a
single project-wide pass once every subsystem is built (a project-wide decision made for
this subsystem, replacing the per-subsystem ERC pass used for analog-io).

## Scope

An I2C GPIO expander driving 6 status LEDs, giving visible operational state (power, MCU
running, radio/link status, fault) without needing a console or app connection. Lives on
LTEBOARD as its second hierarchical child sheet (`status_indication.kicad_sch`), connected
directly to the ESP32-S3 over I2C — unlike every IOBOARD subsystem, no board-to-board header
is involved, since both sheets are on the same physical board.

Not in scope: firmware (register writes, LED driving logic) — hardware only, matching every
other subsystem here; front-panel/enclosure mounting (on-PCB THT LEDs assumed); ERC (deferred
per above).

## Design approach

The GPIO expander itself was already decided during power-subsystem work — NXP PCA9535PW —
and its own supply current was already budgeted into the 3.3V-LOGIC rail in `power.md`. That
budget line covers only the IC itself, not the LEDs it drives (see Step 4).

Key decisions, each made deliberately with reasoning:

1. **Bus topology.** Single I2C segment off the ESP32-S3, PCA9535 the only slave on it.
   `architecture.md`'s bus table already calls this a "shared bus" — intended to also carry
   the analog I/O subsystem's isolated I2C once it crosses the (not-yet-built) board-to-board
   header. Address space doesn't collide (expander at 0x20, ADC/DAC in different ranges), so
   pin selection here (GPIO8/GPIO9) deliberately leaves room for that future connection rather
   than boxing it out.
2. **Address: 0x20.** A0/A1/A2 all tied to `GND_LOGIC`. Only device on the segment today,
   headroom for 7 more if the bus grows.
3. **Pull-ups: 4.7kΩ** on SDA and SCL to `3V3_LOGIC` — standard for a single-slave, short
   on-board bus at 100kHz-400kHz.
4. **LED drive: active-low (GPIO sinks).** GPIO → LED cathode, LED anode → resistor →
   `3V3_LOGIC`. Chosen because the PCA95xx family's sink capability is specified and
   guaranteed per-pin; source capability is weaker and not something to lean on for 6
   simultaneous channels.
5. **LED color family restricted to red/green/yellow/orange.** Blue and white LEDs have a
   much higher forward voltage (~2.8-3.4V) than red/green/yellow/orange (~1.8-2.2V), leaving
   almost no headroom on a 3.3V rail for the resistor to reliably control current. All 6
   channels use the low-Vf family; distinguished by silkscreen label rather than 6 different
   hues, since only 3-4 hues are realistically distinguishable from that family anyway.
6. **INT pin: pulled up (10kΩ to `3V3_LOGIC`), left unrouted.** Every GPIO on this part is
   configured as an output in this design, so INT never asserts — but leaving an open-drain
   pin fully floating is bad practice. Documented spare, not a silent gap.
7. **Decoupling: 100nF at VDD**, same convention as every IC in this project.
8. **10 spare GPIOs (P06, P07, P10-P17) left unconnected, flagged with No-Connect.** 6 of 16
   expander pins cover every subsystem class that needs a status indicator (power, MCU alive,
   three network paths, generic fault) without inventing per-channel LEDs for things already
   visible at the other end of a link. No-Connect flags mark this as deliberate headroom, not
   an oversight.

**Real gap surfaced, not silently absorbed:** `power.md`'s existing PCA9535 budget line
(~0.03-0.2mA) covers only the IC itself. The 6 LEDs add up to ~11.5mA worst-case (Step 4) that
wasn't in that budget yet — same category of gap as the isolated 15V rail found during
analog-io. **Done 2026-09-10** — added to `power.md`'s 3.3V-LOGIC load table and rail total
(see that file's revision history); this took longer than the analog-io rail addition to
actually land (that one was done same-day, 2026-09-08), caught during a later project-wide
documentation consistency pass rather than promptly after this subsystem closed.

## Steps

1. Requirements & scope.
2. Part selection.
3. Design approach.
4. Component math.
5. Schematic capture (KiCad).
6. Acceptance criteria.
7. Documentation & BOM.
8. Sign-off.

---

## Step 1 results: requirements & scope (2026-09-07)

Confirmed via `architecture.md`'s block diagram: the I2C GPIO expander + status LEDs (IOEXP)
sit inside the LTEBOARD subgraph, connected `MCU -- "I2C" --> IOEXP` — same board, no header.
Confirmed via `power.md`'s load-budget table: PCA9535PW already selected, 3.3V-LOGIC rail,
2.3-5.5V supply range. Confirmed via `hardware/kicad/lteboard/lteboard/` directory listing:
only one existing child sheet (`core-compute.kicad_sch`), so this becomes the second.

## Step 2 results: part selection (2026-09-07)

**PCA9535PW pinout — verified against NXP's own datasheet** (not assumed from memory), since
an earlier assumed pinout in this session turned out wrong on the first attempt:

| Pin | Name | Pin | Name |
|---|---|---|---|
| 1 | INT | 13 | IO1_0 |
| 2 | A1 | 14 | IO1_1 |
| 3 | A2 | 15 | IO1_2 |
| 4 | IO0_0 | 16 | IO1_3 |
| 5 | IO0_1 | 17 | IO1_4 |
| 6 | IO0_2 | 18 | IO1_5 |
| 7 | IO0_3 | 19 | IO1_6 |
| 8 | IO0_4 | 20 | IO1_7 |
| 9 | IO0_5 | 21 | A0 |
| 10 | IO0_6 | 22 | SCL |
| 11 | IO0_7 | 23 | SDA |
| 12 | VSS (GND) | 24 | VDD |

Not in KiCad's default library — imported (symbol + footprint + 3D model) under
`hardware/kicad/lteboard/libs/PCA9535PW/`, referenced by a new project-level `sym-lib-table`
using a `${KIPRJMOD}`-relative path (fixed from an absolute Windows path the import tool
wrote by default — would have broken on any other machine or username).

**LED channels — 6 of 16 GPIOs used, 10 spare/documented:**

| # | Signal | Purpose |
|---|---|---|
| 1 | PWR-OK | 3.3V-LOGIC rail present |
| 2 | HEARTBEAT | MCU running (firmware blink) |
| 3 | LTE | Modem registered/connected |
| 4 | WIFI | WiFi connected |
| 5 | ETH | Ethernet link up |
| 6 | FAULT | Any subsystem error (firmware-aggregated) |

## Step 3 results: design approach

See "Design approach" above — bus topology, address, pull-ups, drive polarity, LED color
restriction, INT handling, decoupling, and spare-pin policy were all locked here before any
schematic work started.

## Step 4 results: component math (2026-09-07)

**PCA9535 sink capability — verified against NXP's datasheet:** guaranteed IOL ≈ 10mA/pin at
VOL = 0.5V, absolute max 25mA/pin, 100mA per 8-bit port, 200mA per device.

**LED resistor (all 6 channels, same value):**
- Rail: 3V3_LOGIC = 3.3V. LED: red/green/yellow/orange family, Vf ≈ 2.0V. Target: 2mA.
- R = (3.3V − 2.0V) / 2mA = 650Ω → nearest E24: **680Ω**.
- Actual current at 680Ω: 1.3V / 680Ω = **1.91mA** — well under the 10mA guaranteed sink.
- Worst case (all 6 lit): 6 × 1.91mA ≈ **11.5mA** added to 3V3_LOGIC — the number that needs
  to go into `power.md`'s budget on `main` (see Design approach).

**I2C pull-ups: 4.7kΩ.** Sanity check against the I2C spec's 1000ns max rise-time
(Standard-mode): tr ≈ 0.847 × R × Cb allows up to ~250pF of bus capacitance at 4.7kΩ — a
single on-board device with short traces is nowhere near that.

**INT pull-up: 10kΩ** — not current-critical, just needs to not float.

## Step 5 results: schematic capture (2026-09-07)

**`status_indication.kicad_sch`** — PCA9535PW (U2), 6× LED, 6× 680Ω, 2× 4.7kΩ, 1× 10kΩ,
1× 100nF. Full connectivity verified programmatically (parsed the actual file's symbol
library pin geometry, wire endpoints, and label positions, then reconstructed every net) —
not just visual inspection. This caught two real bugs before commit:

- **All 6 LEDs were initially wired with reversed polarity** (GPIO → anode, cathode →
  resistor → 3V3_LOGIC). This configuration can never light: the cathode sits near 3.3V
  (pulled there through the resistor) with no path to a lower potential, so the diode can
  never see forward bias regardless of GPIO state. Fixed by rotating each LED 180° in place
  (wires stayed anchored, so no rewiring was needed — rotation alone swapped which pin sits
  at which already-wired point).
- **A new hierarchical `3V3_LOGIC` label on `core-compute.kicad_sch` was placed floating**,
  not touching any wire — same net name as the sheet's existing local label, but a label only
  joins a net if its anchor point actually touches a wire or pin. Fixed with a short wire tying
  it into the existing net.

**Cross-sheet work in `core-compute.kicad_sch`:**
- Converted 5 existing local `GND_LOGIC` labels to `global_label` — they were local, which
  would have left this new sheet's ground electrically isolated from the MCU's. Same class of
  bug found and fixed in `power.kicad_sch` during analog-io's ERC pass.
- Added hierarchical labels: GPIO8 (pin 12) → `I2C_SDA` (Bidirectional — I2C is a shared,
  open-drain bus, not simplex, so Input/Output would misdescribe it), GPIO9 (pin 17) →
  `I2C_SCL` (Bidirectional), and a new `3V3_LOGIC` (Passive — neither sheet actually
  generates 3.3V, both are just consumers, so a directional shape would misdescribe this too).
- Confirmed J1's 4 VBUS pins (A4/A9/B4/B9) all share one schematic pin location by symbol
  design (same for its 4 GND pins) — one No-Connect flag there correctly covers all 4; VBUS
  stays unconnected per core-compute's existing "data-only, no VBUS power path" decision.

**`lteboard.kicad_sch`:** new `status_indication` sheet symbol added alongside `core-compute`.
Sheet pins on both symbols (I2C_SDA, I2C_SCL, 3V3_LOGIC) verified to match in both name and
electrical type (Passive/Passive, Bidirectional/Bidirectional ×2), and the 3 connecting wires
verified to land exactly on the corresponding pin coordinates on each side — not just visually
adjacent.

**Label type/shape audit across both sheets** (full table, not spot-checked): GND_LOGIC
global+input on both sheets; 3V3_LOGIC passive on both sheets; I2C_SDA/I2C_SCL bidirectional
on both sheets. No mismatches found.

10 spare GPIOs (P06, P07, P10-P17) marked with No-Connect flags.

## Step 6: Acceptance criteria

1. I2C address 0x20 responds correctly and doesn't collide with any other device once the
   analog I/O subsystem's isolated bus eventually shares this segment — confirmable at
   bring-up.
2. Each LED lights only when its GPIO is driven low, stays off otherwise (active-low, verified
   by design math and connectivity above; confirmable physically at bring-up).
3. No floating I2C or INT pins — met (pull-ups verified above).
4. LED current addition (~11.5mA worst-case) reflected in `power.md`'s budget before this
   subsystem merges to `main` — **Done 2026-09-10** (see Design approach).
5. ERC clean, once the project-wide ERC pass happens (not blocking this subsystem's own
   progress).

Items 1-2 need real hardware — tracked below as commissioning items, same pattern as every
other subsystem.

## Step 7: Bill of materials

See `hardware/bom/status_indication_bom.csv`. 10 parts: U2 (PCA9535PW), 6× LED (D_PWR, D_HB,
D_LTE, D_WIFI, D_ETH, D_FLT), R7/R8/R9/R10/R11/R13 (680Ω LED resistors), R12/R14 (4.7kΩ I2C
pull-ups), R15 (10kΩ INT pull-up), C4 (100nF decoupling). All resistors/cap are generic E24
values; U2 and the LEDs are the only parts needing a specific manufacturer/part match.

## Commissioning test items (Rev-A bring-up)

| Item | What to check | Why not closed now |
|---|---|---|
| I2C address/bus sharing | Confirm no address or bus-timing conflict once the analog I/O subsystem's isolated I2C actually lands on this same segment | That connection doesn't exist yet — deferred to the board-to-board header milestone (roadmap step 7) |
| LED brightness/visibility | Confirm 1.91mA per channel is actually visible at the intended viewing distance | Depends on the specific LED part's efficiency, not knowable on paper |
| PCA9535PW footprint | Confirm the imported footprint matches the real part's mechanical drawing before board fab | Imported from a third-party library, not independently checked, same caution applied to analog-io's and relay-outputs' imported parts |

## Step 8: Sign-off

Status-indication subsystem schematic capture complete on both sheets, cross-sheet link to
`core-compute.kicad_sch` verified pin-by-pin and type-by-type. Two real bugs (reversed LED
polarity on all 6 channels, a floating hierarchical label) and one pre-existing bug outside
this sheet (local instead of global `GND_LOGIC` in `core-compute.kicad_sch`) were caught by
full programmatic connectivity verification and fixed before commit. ERC deferred to a single
project-wide pass once every subsystem is built, per current project decision. LED current
addition to `power.md`'s budget — flagged for `main` at the time, done 2026-09-10.

**Next:** roadmap continues to RS485 (isolated) as the next subsystem.

## Revision history

| Date | Change |
|---|---|
| 2026-09-07 | Scope, part selection, design approach, and component math locked; schematic capture completed on both sheets with cross-sheet link to core-compute; two wiring bugs and one pre-existing local/global label bug caught and fixed; acceptance criteria, BOM, and sign-off completed |
| 2026-09-10 | Closed the one item this doc had carried forward as "not yet done": the ~11.5mA status-LED current addition to `power.md`'s 3.3V-LOGIC budget, added during a project-wide documentation consistency pass. Updated Design approach, Step 6 acceptance criteria item 4, and Step 8 sign-off to reflect this. |
