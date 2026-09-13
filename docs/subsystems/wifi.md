# WiFi Subsystem — Build Plan

**Status:** Plan drafted 2026-09-13, awaiting Ozcan's confirmation before schematic capture.

## Scope

The ESP32-S3-WROOM-1U-N16R8 module (already the fixed, standing MCU choice —
`architecture.md`, `core-compute.md`) has its 2.4GHz WiFi radio built in; there is no
separate WiFi IC to select or wire. What's actually left to design is the **antenna path**:
the module needs an external antenna (the "-1U" suffix means no onboard PCB antenna — an
external one is mandatory), and that path — connector, cable, antenna — was never wired
because `core-compute.md` deliberately deferred it to its own subsystem (see that doc's
Step 6 note). Not in scope: anything about the radio itself (already fixed at the module
level), GPIO/pin assignment (WiFi is native to the SoC — `architecture.md`'s bus table
already lists it as "no external pins, antenna only," confirmed below), the LTE modem's own
antenna (separate subsystem, same pattern, done later).

## Design approach

Four decisions:

1. **Radio: ESP32-S3-WROOM-1U's built-in WiFi, no separate IC — already fixed, not
   re-litigated.** Re-confirmed directly against Espressif's own module datasheet
   2026-09-13 (not re-derived from memory): 802.11b/g/n, 2.4GHz only. Peak TX current
   355mA (802.11b, 1Mbps, @20.5dBm — the single highest current draw across every RF mode
   in the datasheet's own table, Bluetooth LE included) — this is the exact figure
   `power.md` already budgets for the 3.3V-LOGIC rail; re-checking it against the real
   datasheet table this time (not just the earlier general figure) confirms it's correct
   as-is, no change needed there.
2. **Antenna path: module's onboard U.FL jack → U.FL-to-SMA coax pigtail → PCB edge-mount
   SMA female jack, no PCB RF trace.** Confirmed against the reference-hardware LTEBOARD
   photos (`reference-photos/2026-09-08-hardware-assembly-photos/photo_20`, `photo_21`,
   `photo_23`): a short black coax jumper runs from the WROOM-1U module's built-in U.FL
   connector directly to a PCB-mounted SMA female jack at the board edge (2-hole flange,
   through-hole solder tabs) — the identical pattern is used a second time for the LTE
   modem's own antenna, confirming this is this project's established antenna-interface
   style, not a one-off. **The RF signal never touches a PCB trace** — the coax pigtail
   carries it directly from the module to the connector, so there's no controlled-impedance
   microstrip to route or tune on this board. That's a deliberate simplification worth
   keeping: it avoids RF layout work this project has no need to take on, at the cost of
   one extra cable assembly in the BOM. Espressif's own datasheet lists the module's
   antenna connector as U.FL-series-compatible (Hirose U.FL / I-PEX MHF-I / Amphenol AMC —
   all mechanically interchangeable), consistent with the pigtail's plug end.
   **On the schematic:** the SMA jack gets its own symbol for BOM/reference purposes, but
   carries no net to anything else on this sheet — its center (RF) pin is fed externally by
   the pigtail, not by a PCB net, so it's marked No Connect; its shell/ground tab is wired
   to `EARTH`. Reasoning for `EARTH` over `GND_LOGIC`: this connector is externally-facing
   (the antenna sticks outside the enclosure, same as Ethernet's RJ45 and RS485's field
   terminals) — this project already routes every externally-facing connector's
   shield/chassis-bond point to `EARTH` for ESD/surge consistency (RS485's GDT third
   electrode, Ethernet's magjack shield), and there's no reason to treat this port
   differently just because it doesn't carry a wired signal.
3. **Antenna gain: must stay at or under 2.33dBi, per the module's own certification —
   this is a real constraint, not a preference.** Read directly out of Espressif's
   WROOM-1U datasheet 2026-09-13: the module's FCC/CE certification was tested with a
   2.33dBi reference antenna, and the datasheet says explicitly that a higher-gain or
   different-type antenna "may require additional testing, such as EMC." A generic 3dBi
   SMA rubber-duck antenna (e.g. HyperLink HG2403RD-SM, a common and otherwise perfectly
   good part) would exceed that ceiling. Recommending a compliant low-gain antenna instead
   — e.g. Waveshare's "SMA 2.4G 2DB Antenna" (2dBi, SMA-male, 2.4-2.5GHz) — to stay inside
   the certified envelope with margin, matching the board's SMA-female jack (regular SMA,
   not reverse-polarity — confirmed from the photos, the board-side connector's center pin
   is a socket, standard SMA-female convention). This is easy to miss (a bigger-looking
   antenna spec sheet doesn't flag the certification link at all) and worth flagging
   clearly rather than defaulting to whatever antenna looks most capable.
4. **Power: `3V3_LOGIC` / `GND_LOGIC`, already budgeted — no new decision.** Covered under
   decision 1 above; nothing new for `power.md`.

## Step 1 results: antenna path components (drafted 2026-09-13)

| Parameter | Value | Source |
|---|---|---|
| Antenna connector (module side) | U.FL / I-PEX MHF-I / Amphenol AMC compatible, built into the WROOM-1U module | Espressif WROOM-1U datasheet |
| Pigtail | U.FL plug → SMA female bulkhead, RG178, ~10-15cm | Reference-hardware photos (physical pattern); generic part, no specific MPN required |
| Board connector | SMA female jack, 2-hole flange, PCB/panel mount, 50Ω | Reference-hardware photos; representative real part: [Amphenol RF 132163](https://www.amphenolrf.com/en-us/part/132163/1014/) |
| Antenna | 2.4-2.5GHz, SMA-male, ≤2.33dBi gain (certification limit) | Espressif WROOM-1U datasheet (gain ceiling); representative real part: Waveshare "SMA 2.4G 2DB Antenna" (2dBi) |
| Board-side grounding | SMA jack shell/flange → `EARTH` (decision 2); center pin → No Connect | This project's established externally-facing-connector convention |
| RF routing | None — coax pigtail carries the signal, no PCB trace | Confirmed against reference-hardware photos |

**Relevant datasheet figures, WROOM-1U (confirmed 2026-09-13):**

| Item | Value |
|---|---|
| WiFi standards | 802.11b/g/n, 2.4GHz only |
| Max TX power | 20.5dBm (802.11b, 1/11Mbps) |
| Peak TX current | 355mA (802.11b, 1Mbps @ 20.5dBm — worst case across all RF modes incl. BLE) |
| Certified reference antenna gain | 2.33dBi (do not exceed without expecting to redo EMC testing) |
| Antenna connector family | U.FL (Hirose) / MHF-I (I-PEX) / AMC (Amphenol) — mechanically interchangeable |

## Step 2: schematic capture — not started, next up once confirmed

Single addition to `hardware/kicad/lteboard/lteboard/core-compute.kicad_sch` (this lives
next to the module it belongs to, not a separate sheet — there's no bus/signal to carry
across a hierarchical boundary, just one connector): place the SMA jack symbol, tie its
ground pin to `EARTH` (a new global label on this sheet if not already present — same
label used by Ethernet, so should already be declared project-wide), mark its RF pin No
Connect, and note in the schematic (or here) that the pigtail is a physical assembly, not a
netlist connection. That's the entire wiring task for this subsystem.

## Step 3: verification checklist / ERC — not run yet

Deferred to the single end-of-project ERC pass, same policy as every subsystem since RS232.
One thing to double check there specifically: a deliberately-unconnected RF pin marked No
Connect shouldn't raise an ERC error, but confirm KiCad accepts the NC marker on this
symbol's pin the same way it does elsewhere on this project.

## Step 4: acceptance criteria — draft

1. Module's 3.3V-LOGIC draw stays within the already-budgeted 355mA WiFi TX peak — no
   change needed in `power.md` (re-verified this session).
2. SMA jack's shell continuity to `EARTH` — visual/continuity check at assembly.
3. WiFi associates with a real access point at expected range/RSSI with the chosen antenna
   — bench item, can't close on paper.
4. If range/RSSI at bring-up disappoints, the antenna (2dBi, off-board, screw-on) can be
   swapped for another SMA-male 2.4GHz antenna without any board change — as long as its
   gain stays at or under 2.33dBi to keep the module's existing certification valid.

All of 2-4 need real hardware — Rev-A bring-up, same pattern as every other subsystem.

## Step 5: documentation & BOM — not done yet

Will add a WiFi subsystem section to `hardware/datasheets/README.md` once the schematic
placement (Step 2) is done and Ozcan confirms the connector's actual reference designator.
BOM: the SMA jack (on-board), the pigtail cable (mechanical assembly, BOM'd but not a
schematic symbol), and the antenna (off-board accessory, BOM'd for completeness the same
way this project already BOMs connector-adjacent parts that never mount to the PCB itself).

## Step 6: sign-off — not yet

Pending confirmation, Step 2 (schematic capture), and Steps 3-5.

## Revision history

| Date | Change |
|---|---|
| 2026-09-13 | Doc created. Antenna-path topology (module U.FL → coax pigtail → PCB-mount SMA jack, no PCB RF trace) confirmed against the reference-hardware LTEBOARD photos, cross-checked against Espressif's own WROOM-1U datasheet (antenna connector family, TX power/current, and — the one easy-to-miss finding — the 2.33dBi certified-antenna gain ceiling, which rules out a generic 3dBi rubber-duck antenna without extra EMC testing). Re-verified the 355mA WiFi-TX-peak figure already used in `power.md`/`core-compute.md` directly against the datasheet's own current-consumption table (it's the correct worst-case row, 802.11b @ 20.5dBm — no error, no change needed). SMA jack's shield/ground assigned to `EARTH` per this project's existing externally-facing-connector convention (same as Ethernet's magjack shield, RS485's GDT third electrode). |
