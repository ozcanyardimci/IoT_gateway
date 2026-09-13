# Project memory — IoT_gateway

Working notes for continuity across sessions. Not part of the public-facing project —
kept out of git (see `.gitignore`).

## Non-negotiable policy

Public-facing project content (README, repo files, commit messages, code, docs) never
mentions the real reference product or the words "reverse engineering." Public framing is
"designed independently from scratch." This applies to anything committed/pushed. This
file itself, being untracked, isn't public-facing, but don't introduce the real product
name into anything that gets committed. Do not relitigate this constraint.

## Who this is for

Ozcan — university student, no prior EE/CAD/embedded background before this project.
Building a 4G IoT gateway from scratch as a hands-on learning vehicle for electronics,
KiCad, and firmware. He does the hands-on engineering himself — schematic wiring, part
placement, running simulations in KiCad. Claude's role is research, verification, catching
errors, explaining concepts, and writing docs — not doing the hands-on design work for him
unless he explicitly hands it off (default assumption: he wants to do it himself).

## Working style

- Short, direct answers. Necessary information only, no padding.
- Confirm understanding briefly before big multi-step work, then proceed.
- When giving instructions for him to execute in KiCad: explain clearly but don't dump
  everything at once — he's asked to "slow down" before.
- He checks correctness at multiple checkpoints (screenshots of schematic state) — expect
  this pattern to continue, and actually check carefully each time; real errors have been
  caught this way repeatedly (wrong part value, unset resistor value, wrong reference
  designator).
- Verify claims against real datasheets, not assumption or memory. Flag confidence level
  honestly when a datasheet gap exists (e.g. W5500 max current — not published, margin
  used instead).
- Git workflow: plan a subsystem → push the plan → branch `subsystem/<name>` → execute →
  merge to `main` on sign-off.
- **Git execution (changed 2026-09-10):** Ozcan runs every git command himself, from add
  through commit, merge, and push. Claude never runs git via the device bridge — it only
  supplies the exact command(s) to run, including the commit message with the required
  trailers. Reason: the device bridge's shell (`device_bash`) hit a persistent
  sandbox-startup failure that survived app restart, WSL2 restart, and a full computer
  reboot — file staging/commit still works, but the shell doesn't, so this is also just the
  reliable path regardless of bridge health.
- **Commit message length (changed 2026-09-10):** short, not long. One line summary, no
  multi-paragraph rationale essay in the commit body — save the "why" for the subsystem
  doc's own revision history, which already carries it.
- Commit messages need a Co-Authored-By + Claude-Session trailer — the exact format is
  injected per-session via system reminder; use whatever the current session specifies,
  don't hardcode an old one.
- Project docs should read as written by an experienced engineer — avoid inline
  self-narrating language ("not guessed, verified" style hedging), avoid dated
  "Correction:" sections scattered through prose (use a revision-history table instead).

## Architecture

Two physically separate PCBs, board-to-board header, split for noise isolation:
- **Communications board (LTEBOARD):** ESP32-S3 (MCU + WiFi), Quectel EG915U-EU (LTE),
  WIZnet W5500 (Ethernet), PCA9535PW (I2C GPIO expander, status LEDs).
- **I/O board (IOBOARD):** power regulation, relay outputs, digital inputs, analog I/O,
  RS485, RS232.

Full block diagram and bus assignment: `docs/architecture.md`.

## Build methodology

Subsystem-first: each subsystem gets its own build plan under `docs/subsystems/`,
simulation-validated (KiCad/ngspice) before schematic consolidation, ahead of a Rev-A
prototype PCB. See `docs/roadmap.md` for the top-level sequence and status.

Per-subsystem plan shape (adapt per subsystem, don't copy literally — see chat 2026-09-03):
requirements → load/interface budget → protection/isolation design → part selection →
schematic capture → simulation/verification → margin check → power sequencing/interface
check → acceptance criteria → BOM → sign-off.

Subsystem order: power (done) → core compute → digital inputs → relay outputs → status
indication → analog I/O → RS485 → RS232 → Ethernet → WiFi → LTE (last, most complex).

## Status by subsystem

All statuses below reconciled against each subsystem's own doc 2026-09-10 (a project-wide
documentation consistency pass — several of these were stale here and in `docs/roadmap.md`/
`README.md` relative to work actually completed; see each file's own revision history for
what was corrected).

- **Power** — DONE. Full protection chain (fuse → LM74610-Q1 ideal diode + CSD18531Q5A
  MOSFET → TVS3300 Flat-Clamp surge protection) into 5 independent rails (3.3V-LOGIC,
  3.3V-LTE, 5V-RELAY, 3.3V-ANALOG-ISO, 15V-ANALOG-ISO). Inrush simulated in ngspice (161A
  worst case, 2.5x margin on Q1, no NTC needed). Schematic complete in
  `hardware/kicad/ioboard/`. Full writeup: `docs/subsystems/power.md`.
- **Core compute (ESP32-S3)** — DONE, all 11 plan steps closed. Power/decoupling, EN reset,
  GPIO0 boot circuit, USB-C (data-only). Schematic: `hardware/kicad/lteboard/lteboard/
  core-compute.kicad_sch`. Full writeup: `docs/subsystems/core-compute.md`.
- **Digital inputs (8x, opto-isolated)** — DONE, all 9 plan steps closed. Two LTV-247 ICs,
  2.4kΩ current-limit + 1N4148 reverse protection per channel. ERC deferred to pre-merge
  (see Open items). Full writeup: `docs/subsystems/digital-inputs.md`.
- **Relay outputs (4x)** — DONE, all 8 plan steps closed. MMBT3904 driver stage per
  channel, ALDP105 relay, 1N4148 flyback. ERC + ALDP105 footprint check deferred to
  pre-merge (see Open items). Full writeup: `docs/subsystems/relay-outputs.md`.
- **Status indication (I2C GPIO expander + LEDs)** — DONE, all 8 plan steps closed.
  PCA9535PW driving 6 status LEDs over I2C, on LTEBOARD (no board-to-board header
  involved). ERC deferred to a single project-wide pass (this subsystem's own choice,
  rather than the per-subsystem pre-merge pattern used elsewhere). Full writeup:
  `docs/subsystems/status-indication.md`.
- **Analog I/O (input + output)** — DONE, all 9 plan steps closed. Isolated ADS1115 (ADC) +
  MCP4725 (DAC) + ISO1540 (I2C isolator) + 2x LM2904, extending isolation to the output per
  a deliberate deviation from the reference design. Added a new 15V-ANALOG-ISO rail to
  `power.md` (Recom RK-0515S — note: this doc's own Step 1 candidate, R05P215S, was
  superseded by that actual pick; see `docs/subsystems/analog-io.md`'s revision history).
  ERC run to a fully-explained clean state (15 documented exclusions). Full writeup:
  `docs/subsystems/analog-io.md`.
- **RS485** — DONE, all 8 plan steps closed 2026-09-10. Copies Mornsun's own Fig. 2
  harsh-environment reference circuit for the TD321S485H-A transceiver exactly (GDT +
  series R + TVS + common-mode choke + external bias, 4.7kΩ). ERC deliberately not run
  before closing the doc — see Open items below. Full writeup: `docs/subsystems/rs485.md`.
- **RS232** — DONE, all 8 plan steps closed 2026-09-10. TI MAX3232EIPWR front end, no added
  protection network, Phoenix Contact MC 1,5/3-ST-3,5 connector reused from RS485. ERC
  deliberately not run before closing the doc — see Open items below. Full writeup:
  `docs/subsystems/rs232.md`.
- **Ethernet (W5500)** — schematic capture (Step 4) done 2026-09-13, all 7 design decisions
  locked. Würth 7499010441 magjack's exact part number and connector type visually
  re-confirmed against reference-hardware photos; its termination network is now CONFIRMED
  (not just high confidence) as integrated, no external network needed — read the actual
  datasheet directly, pin 8 (GND) + both shield pins are one internal node and all go to
  `EARTH`. Four intentional additions beyond the original plan (ferrite-bead `3V3A` rail,
  crystal bias/damping resistors, optional 0Ω TX/RX line damping resistors, second bulk cap
  + GND-EARTH bridge cap) — all reference-schematic-sourced and confirmed sound. One open
  item: a value-label typo (see Open items below). ERC and BOM still pending. Full writeup:
  `docs/subsystems/ethernet.md`.
- WiFi, LTE — not started.

## Key engineering decisions worth remembering

- **Grounding:** GND_LOGIC (shared, non-isolated, crosses the board-to-board header) vs.
  GND_ANALOG_ISO / GND_FIELD_DI / GND_RS485_ISO (isolated, stay local to IOBOARD, never
  tied to GND_LOGIC). Chassis ground single-point-bonded to GND_LOGIC at the power entry.
- **Analog input isolation** was added as a deliberate improvement over the reference
  design (which has none) — own isolated supply rail, own isolation amp/ADC.
- **KiCad labeling:** GND_LOGIC uses a Global Label (project-wide). The other 5 rail/ground
  nets use Hierarchical Label + Sheet Pin (Output direction) — Ozcan's explicit preference
  over defaulting everything to Global Labels.
- Commissioning-test pattern: anything that can't be verified on paper (real supply
  impedance, W5500/LTE actual current draw) gets conservative margin now + a tracked
  bench-measurement item for Rev-A, not chased indefinitely on paper.

## File map

- `README.md` — project overview
- `docs/architecture.md` — block diagram, bus assignment, fixed MCU constraints
- `docs/roadmap.md` — top-level 10-step plan and subsystem order/status
- `docs/build-log.md` — incremental build/bring-up log
- `docs/subsystems/power.md` — full power subsystem writeup (reference for doc style/depth
  on future subsystems)
- `hardware/datasheets/README.md` — datasheet/app-note links per subsystem, no PDFs stored
- `hardware/kicad/README.md`, `hardware/kicad/ioboard/`, `hardware/kicad/lteboard/` —
  KiCad projects
- `firmware/` — PlatformIO project (ESP32-S3, pioarduino platform)

## Open items

- **ERC policy (confirmed 2026-09-10):** run once, project-wide, after all subsystems'
  connections are done — not per-subsystem before each merge. Applies to digital-inputs,
  relay-outputs, rs485, and rs232 so far (status-indication already used this same
  approach independently). This is a deliberate decision, not a gap in any of those
  subsystem docs, which all close out DONE without ERC having been run yet.
- **Digital-inputs:** assign real footprints to the passives, diodes, and J2 (only U7/U8
  have one so far). Deferred deliberately by the user (2026-09-05) — footprint assignment
  naturally belongs at the layout stage (roadmap step 9).
- **Relay-outputs:** footprints for R26-R33, D9-D12, J3, C21-C22 (no footprint yet, same
  deferral). K1-K4 and Q1-Q4 already have footprints (`ALDP105:RELAY_ALDP105` bundled with
  the imported ALDP105 symbol library, `Package_TO_SOT_SMD:SOT-23` from KiCad's standard
  library) but the ALDP105 one specifically has NOT been independently checked against the
  real part's mechanical drawing — it came from a third-party import, not verified
  pin-spacing. Confirm that one for real before board fab, don't just trust it because it's
  already filled in.
- **RS485:** one specific thing for the end-of-project ERC pass to settle: U14
  (common-mode choke) pin 2 appeared unconnected and pin 3 appeared shorted onto pin 4's
  node during manual (non-ERC) schematic verification — checked with no mirror/rotation
  ambiguity on that component, so not a tooling artifact like some other findings on this
  sheet turned out to be. Ozcan reviewed and considers the wiring correct as drawn; ERC will
  resolve it objectively either way. Also: `RS485_RXD` hierarchical label is shape `input`
  on both the child sheet and the root sheet pin — should be `output` (signal leaves this
  sheet toward the MCU) per the same rule RS232 got right; non-blocking, left for the ERC
  pass to flag formally.
- **RS232:** no known open items beyond the shared ERC pass above — footprints deferred to
  layout stage same as everywhere else (J6 currently blank).
- **Ethernet — schematic done, two small items remain:** (1) C7's Value field reads
  `1nF/2kW`, should be `1nF/2kV` — cosmetic label typo, doesn't affect the netlist, fix
  before BOM/ordering. (2) `hardware/bom/ethernet_bom.csv` not written yet — unblocked now
  that real reference designators exist. Termination network (decision 4) is fully resolved
  and CONFIRMED via direct datasheet read, no longer an open item.
- Toshiba SSM3J307T sourcing caveat (superseded part, low priority, likely fine to drop
  without a direct-source re-check).
- Two non-blocking architecture-diagram polish items flagged early in the project: missing
  USB debug node in the Mermaid diagram, oversimplified power block — still pending,
  revisit when convenient.
