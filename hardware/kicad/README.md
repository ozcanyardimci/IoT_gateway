# KiCad projects

Two boards, two KiCad projects, matching the two-board architecture (noise isolation
between the communications board and the I/O board).

- **ioboard/** — power, relay outputs, digital inputs, analog I/O, RS485, RS232.
  `ioboard.kicad_pro` was created first since power is the first subsystem designed and
  lives entirely on this board.
- **lteboard/** — ESP32-S3 core compute, WiFi, LTE, Ethernet, status indication (I2C GPIO
  expander). Created once LTEBOARD subsystems start.

Each project uses hierarchical sub-sheets, one per subsystem (e.g. `power.kicad_sch`,
`relay_outputs.kicad_sch`), added to the board's root sheet as that subsystem reaches
schematic capture — matches the incremental-consolidation approach in `docs/roadmap.md`,
step 7.
