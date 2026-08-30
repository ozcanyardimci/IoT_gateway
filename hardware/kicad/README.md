# KiCad projects

Two separate boards, two separate KiCad projects — matches the two-board architecture
decided early in this project (noise isolation between the communications board and the
I/O board).

- **ioboard/** — the I/O board: power, relay outputs, digital inputs, analog I/O, RS485,
  RS232. Create the KiCad project here first (`ioboard.kicad_pro`), since power is the
  first subsystem being designed and it lives entirely on this board.
- **lteboard/** — the communications board: ESP32-S3 core compute, WiFi, LTE, Ethernet,
  status indication (I2C GPIO expander). Created later, once LTEBOARD subsystems start.

Each project uses hierarchical sub-sheets, one per subsystem (e.g. `power.kicad_sch`,
`relay_outputs.kicad_sch`), added to the relevant board's root sheet as that subsystem's
design work reaches schematic capture — matches the "incremental consolidation" approach in
the main roadmap (docs/roadmap.md, step 7).
