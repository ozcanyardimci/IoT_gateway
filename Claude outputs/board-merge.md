# Board merge — `ioboard+lteboard`

## Status

DONE (2026-09-19). `hardware/kicad/ioboard+lteboard/` is now the single active KiCad
project for all future schematic work. `ioboard/` and `lteboard/` are frozen as of this
date — not edited further, kept only as the historical record of each board's schematic
capture up to the point of merge.

## Why

Steps 6 (fine-grained pin/resource planning) and 7 (board-to-board wiring) both require
working across both boards at once — assigning MCU pins that serve subsystems living on
the other physical board, and wiring the rails/signals that cross the board-to-board
connector. Doing that from two separate, closed KiCad projects meant reasoning about
cross-board connectivity from memory/documentation rather than seeing it directly. One
merged schematic project makes the cross-board picture direct and checkable.

## What was done

All 10 real subsystem sheets were imported as hierarchical sheet symbols into a new root
sheet, `ioboard+lteboard.kicad_sch` (WiFi has no sheet of its own — it lives inside
`core-compute.kicad_sch`, unchanged by the merge):

- IOBOARD side: `power`, `analog_io`, `digital_inputs`, `relay_outputs`, `rs232`, `rs485`
- LTEBOARD side: `core-compute`, `ethernet`, `lte`, `status_indication`

Each subsystem's sheet-symbol pins were wired directly on the root canvas wherever the
corresponding original board's own root sheet already tied them together (see "Verified"
below). Two graphic borders + text labels ("IOBOARD" / "LTEBOARD") mark which sheet
symbols belong to which physical board — visual grouping only, no electrical meaning.

KiCad physically copied each subsystem `.kicad_sch` file into the new project folder
rather than referencing the originals in place. Content was verified identical to the
originals at merge time — component count, wire count, hierarchical/global label count,
junction count, and no-connect count all match exactly between each original file and its
copy. The only difference is reference designators, which KiCad auto-renumbered on import
to avoid collisions across the new shared hierarchy; this doesn't matter since a final
Annotate Schematic pass resets numbering anyway once wiring/placement is finished (see
"Reference designators" below). From 2026-09-19 onward, only the copies under
`ioboard+lteboard/` get edited — the ones under `ioboard/` and `lteboard/` will not
receive further changes and should not be treated as current.

## Verified (full netlist reconstruction, 2026-09-19)

Connected, confirmed correct:

- `power` ↔ `relay_outputs`: `5V_RELAY`
- `power` ↔ `analog_io`: `3V3_ANALOG_ISO`, `GND_ANALOG_ISO`, `15V_ANALOG_ISO`
- `core-compute` ↔ `status_indication`: `I2C_SCL`, `I2C_SDA`
- IOBOARD-side `3V3_LOGIC` common across `analog_io`/`digital_inputs`/`power`/`rs232`/
  `rs485`
- LTEBOARD-side `3V3_LOGIC` common across `core-compute`/`ethernet`/`lte`/
  `status_indication`
- `GND_LOGIC` and `EARTH` are Global Labels in the original files — these merge
  project-wide automatically regardless of sheet hierarchy, so no sheet-pin wiring was
  needed for them; confirmed present in every file that should have them

Not yet connected — expected, waiting on later roadmap steps, not new defects found by
this merge:

- IOBOARD-side `3V3_LOGIC` and LTEBOARD-side `3V3_LOGIC` are two separate islands, not
  tied to each other yet — needs the physical board-to-board connector (step 7, not
  started)
- `VBAT_LTE`: isolated on both `power`'s output pin and `lte`'s input pin — same reason,
  step 7
- `LTE_PWRKEY`/`LTE_RESET`/`LTE_RXD`/`LTE_TXD`, `RS485_TXD`/`RS485_RXD`,
  `RS232_TXD`/`RS232_RXD`, and all 6 `ETH_*` SPI pins: isolated because
  `core-compute.kicad_sch` doesn't expose matching GPIO/UART/SPI pins yet (step 6,
  fine-grained pin assignment, not started)

## Known open question for later — PCB layout (steps 8-9)

`ioboard+lteboard` being the sole active schematic project means both physical boards'
component data lives in one project's netlist. KiCad's "Update PCB from Schematic"
imports a project's entire hierarchy with no per-sheet filter, so producing two clean
physical-board PCB files from this one project will need a deliberate approach at that
stage — most likely two `.kicad_pcb` files that each import the full netlist and then
have the other board's footprints removed, repeated after every schematic sync. Not a
blocker now; flagged here so it isn't a surprise at step 8.

An alternative was considered and rejected: keep `ioboard`/`lteboard` as thin per-board
projects whose sheet symbols point at the same underlying subsystem files used by
`ioboard+lteboard` (rather than separate copies), so each board's PCB import would stay
clean automatically with no manual footprint filtering. Ozcan's explicit call
(2026-09-19) was to freeze `ioboard`/`lteboard` as they are and work only from the merged
project going forward — this doc exists so that decision and its known PCB-layout
consequence are both on record, not just the decision.

## Reference designators

Not yet reconciled — expected. Each subsystem was originally annotated independently
(`R1`, `U1`, etc. in each), so the merged hierarchy currently has many duplicate/
placeholder references. Resolved by running Tools → Annotate Schematic across the whole
merged sheet once wiring/placement is finalized — deliberately not run before that, to
avoid re-running it repeatedly mid-work.
