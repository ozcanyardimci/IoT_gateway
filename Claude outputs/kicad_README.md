# KiCad projects

Two physically separate PCBs (noise isolation between the communications board and the
I/O board), captured as one merged KiCad schematic project.

- **ioboard+lteboard/** — the active project. Both boards' subsystem sheets in one
  hierarchy:
  - IOBOARD side: `power`, `analog_io`, `digital_inputs`, `relay_outputs`, `rs232`,
    `rs485`
  - LTEBOARD side: `core-compute` (also carries WiFi — the ESP32-S3-WROOM-1U's radio is
    fixed at the module, WiFi only added an external antenna path here), `ethernet`,
    `lte`, `status_indication`

  The two boards are marked on the root sheet with a graphic border and label around each
  cluster of subsystem sheets — visual grouping only, no electrical meaning. This is
  where all schematic work happens going forward: fine-grained pin assignment, the
  board-to-board interconnect wiring, and anything after. See `docs/board-merge.md` for
  what the merge covered, what's verified connected, and a known open question about
  producing two separate physical-board PCB files from this one project at layout time.

- **ioboard/**, **lteboard/** — the original per-board projects, frozen as of the
  2026-09-19 merge. Kept as the historical record of each board's schematic capture up to
  that point; not edited further.

Each project uses hierarchical sub-sheets, one per subsystem (e.g. `power.kicad_sch`,
`relay_outputs.kicad_sch`).
