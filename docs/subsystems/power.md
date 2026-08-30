# Power subsystem — build plan

Status: **not started** (roadmap step 4a). Simulation only, using pre-made DC-DC modules
(no custom-designed regulator, no breadboard bring-up).

## Scope
Reverse-polarity protection, overcurrent/surge protection, and 3.3V/5V regulation from a
10-30VDC field input, feeding both boards over the board-to-board header.

## Decisions
- **Regulation approach: pre-made DC-DC modules**, not a custom-designed buck converter.
  Faster and lower-risk; trades off some of the "design your own regulator" learning for
  reliability. Two-stage topology expected (pre-regulation step-down, then fixed-output
  point-of-load modules), pending confirmation in step 4.
- **Validation approach: simulation only (ngspice)**, no physical breadboard test. This
  limits what can actually be verified before Rev-A — see the note under step 8.

## Steps

1. **Requirements & specs** — lock the input range (10-30VDC) and required output rails
   (3.3V, 5V). Pull voltage/current requirements for every downstream device from its own
   datasheet.
2. **Load budget** — sum typical and peak current draw per rail, from real datasheet
   numbers rather than estimates. This sizes everything downstream.
3. **Grounding & isolation architecture** — decide whether logic ground, field-power
   ground, and chassis/earth are separate or joined, and where. Settled before parts are
   chosen, since it affects module selection.
4. **Module selection** — choose DC-DC modules whose input range fully covers 10-30V and
   whose current rating carries 20-30% margin over the load budget. Check each candidate's
   thermal derating curve (enclosure ambient, not open-air) and whether the vendor
   publishes a SPICE/behavioral model.
5. **Protection design & sequencing** — fixed order from the input connector: fuse/PTC,
   then reverse-polarity (P-MOSFET), then surge/EMI (TVS + Y-cap), then regulation. Fuse
   sized to the load budget. Surge/EMI margin is designed toward common industrial
   immunity practice (e.g. IEC 61000-4-5-class thinking), without formal certification.
6. **Connector & wiring selection** — terminal block and wire gauge sized to the real
   load, with adequate creepage/clearance for 30V.
7. **Schematic capture (KiCad)** — full input-to-output chain, with test points at every
   stage boundary for later debugging.
8. **ngspice simulation** — validates the protection circuit and loading behavior across
   the full input/load range. Note: since regulation uses off-the-shelf modules, this
   cannot verify the modules' internal control loops — only vendor data and margin
   (step 4) cover that.
9. **Margin verification** — every component's rating checked against worst-case
   conditions, not nominal.
10. **Power sequencing & brown-out check** — confirm whether any downstream device needs a
    specific rail power-up order, and define behavior if input sags briefly (e.g. during
    relay switching) — fail safely, not unpredictably.
11. **Acceptance criteria** — concrete pass/fail numbers, defined before the subsystem is
    marked done.
12. **Documentation & BOM** — final part numbers recorded.
13. **Sign-off** — move to the next subsystem (core compute).

## Known open items carried in from teardown
- No fuse/PTC was identified on the reference board's photos — being added regardless as
  standard practice for a field-powered device (see step 5).
- Relay coil-driver stage between MCU GPIO and relay coil is still unresolved — tracked
  separately under the relay-outputs subsystem, not this one.
