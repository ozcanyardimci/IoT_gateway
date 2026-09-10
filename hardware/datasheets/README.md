# Datasheets, sources & application notes

Links only — no PDF binaries in this repo. Keeps repo size manageable, avoids stale local
copies of documents the manufacturer updates over time, and avoids redistributing
copyrighted PDFs. Updated as each subsystem is worked through.

## Power subsystem

| Part | Role | Datasheet / source |
|---|---|---|
| ESP32-S3-WROOM-1U | Main MCU module | [Espressif datasheet](https://documentation.espressif.com/api/resource/doc/file/3yD6w5Y5/FILE/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf) |
| Quectel EG915U-EU | LTE Cat-1 modem | [Quectel EG915U Series Hardware Design v1.1](https://quectel.com/content/uploads/2024/02/Quectel_EG915U_Series_Hardware_Design_V1.1.pdf) |
| WIZnet W5500 | SPI Ethernet controller | [WIZnet datasheet v1.1.0](https://docs.wiznet.io/img/products/w5500/W5500_ds_v110e.pdf) |
| NXP PCA9535PW | I2C GPIO expander | [NXP datasheet](https://www.nxp.com/docs/en/data-sheet/PCA9535_PCA9535C.pdf) · [TI second source](https://www.ti.com/lit/ds/symlink/pca9535.pdf) |
| MAX3232E | RS232 transceiver | [ADI/Maxim MAX3222E-MAX3246E family datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/max3222e-max3246e.pdf) |
| Mornsun TD321S485H-A | Isolated RS485 module | [Mornsun datasheet](https://www.mornsun-power.com/public/uploads/pdf/TD5(3)21S485H-A.pdf) |
| Panasonic ALDP105 | Relay (x4) | [Panasonic product page](https://na.industrial.panasonic.com/products/relays-contactors/mechanical-power-relays/lineup/general/series/1988/model/136824) · [Future Electronics PDF](https://www1.futureelectronics.com/doc/Panasonic/ALDP105.pdf) |
| Lite-On LTV-247 | Digital-input optocoupler | [Datasheet via LCSC](https://datasheet.lcsc.com/datasheet/pdf/6ec3b014c2c1b1e2589a5b88a431fe52.pdf?productCode=C115451) |
| AME8808 | Analog LDO | AME8808's own sheet isn't available in extractable form — using the AME8805/8813 same-family sheet as reference: [AME datasheet](https://www.ame.com.tw/datasheet/11-AME8805%208813_061313_R.13.pdf) |
| LM2904 | Analog stage op-amp | [Diodes Inc. LM2902/LM2904 datasheet](https://www.diodes.com/assets/Datasheets/LM2902-04.pdf) |

## Power modules & protection components

| Part | Role | Datasheet / source |
|---|---|---|
| Würth MagI3C-VDLM 171013801 | 3.3V-LOGIC and 5V rail buck module (two instances) | [Datasheet](https://www.we-online.com/en/components/products/datasheet/171013801.pdf) |
| Würth MagI3C-VDLM 171033801 | 3.3V-LTE rail buck module, 3A | [Datasheet](https://www.we-online.com/components/products/datasheet/171033801.pdf) |
| Recom R1SX-3.33.3-R | Isolated supply, analog input field side, 3.3V-in/3.3V-out | [Recom datasheet](https://recom-power.com/pdf/Econoline/R1SX.pdf) · [DigiKey](https://www.digikey.com/en/products/detail/recom-power/R1SX-3.33.3-R/6708875) |
| Littelfuse RXEF135 | Input PTC resettable fuse | [Datasheet](https://www.littelfuse.com/assetdocs/resettable-ptc-rxef-datasheet?assetguid=e9a7b6b3-79ce-478c-a39a-0a70ee48ccec) |
| TI LM74610-Q1 | Ideal diode controller, reverse-polarity protection | [Datasheet](https://www.ti.com/lit/ds/symlink/lm74610-q1.pdf) |
| TI CSD18531Q5A | 60V N-channel MOSFET, driven by LM74610-Q1 | [Product page](https://www.ti.com/product/CSD18531Q5A) |
| TI TVS3300 | Input surge TVS, Flat-Clamp technology, 33V standoff, 40V max clamp at 35A | [Datasheet](https://www.ti.com/lit/ds/symlink/tvs3300.pdf) · [Product page](https://www.ti.com/product/TVS3300) |
| Würth WCAP-CSSA 8853522140011 | Input EMI/safety Y-cap | [Datasheet](https://www.we-online.com/components/products/datasheet/8853522140011.pdf) |
| Recom RK-0515S | Isolated 15V DC-DC module, analog output gain-stage supply (U7) | [Recom Econoline family PDF](https://recom-power.com/pdf/Econoline/RK_RH.pdf) — added 2026-09-08 for analog-io's "15V problem"; 3kVDC isolation stated directly, no re-verification gap |

Superseded parts (kept for traceability — see `docs/subsystems/power.md` revision history
for why): Toshiba SSM3J351R,LF (replaced by LM74610-Q1 + CSD18531Q5A), Littelfuse SMBJ36A
(replaced by TVS3300), Recom R1SX-3305 (replaced by R1SX-3.33.3-R — wrong output voltage
for the rail), Recom R05P215S (replaced by RK-0515S for the 15V-ANALOG-ISO rail —
unresolved isolation-rating gap on the R05P215S datasheet pull, see
`docs/subsystems/analog-io.md`'s revision history).

## Core compute subsystem

| Part | Role | Datasheet / source |
|---|---|---|
| Espressif ESP32-S3-WROOM-1U-N16R8 | Main MCU module | [Datasheet](https://documentation.espressif.com/api/resource/doc/file/3yD6w5Y5/FILE/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf) |
| Molex 216989-0001 | USB-C receptacle, USB2.0-only, 14-pin | [DigiKey product page](https://www.digikey.com/en/products/detail/molex/2169890001/13913746) |
| E-Switch TL1150AF070Q | Manual reset/boot push buttons (SW1, SW2) | [DigiKey product page](https://www.digikey.com/en/products/detail/e-switch/TL1150AF070Q/1556582) |

| Document | Covers | Source |
|---|---|---|
| Espressif ESP32-S3 Hardware Design Guidelines — Schematic Checklist | EN RC delay (10k+1uF), GPIO0 pull-up guidance, decoupling values/placement, USB D+/D- series resistor guidance | [Page](https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32s3/schematic-checklist.html) |
| esptool documentation — Boot Mode Selection (ESP32-S3) | GPIO0's internal 45k pull-up and the need for an external 10k pull-down (not pull-up) to reliably enter download mode | [Page](https://docs.espressif.com/projects/esptool/en/latest/esp32s3/advanced-topics/boot-mode-selection.html) |

## Digital inputs subsystem

| Part | Role | Datasheet / source |
|---|---|---|
| Lite-On LTV-247 | 4-channel opto-isolator, digital inputs (x2 for 8ch) | [DigiKey product page](https://www.digikey.com/en/products/detail/liteon/LTV-247/4307982) - [LCSC product page](https://www.lcsc.com/product-detail/SMD-Optocouplers_LTV-247_C115451.html) - [Lite-On LTV-2X7 family datasheet](https://datasheet.lcsc.com/datasheet/pdf/6ec3b014c2c1b1e2589a5b88a431fe52.pdf?productCode=C115451) |
| Phoenix Contact MC 1,5/9-ST-3,5 | 8-channel + common field connector (J2) | [Newark product page](https://www.newark.com/phoenix-contact/mc-1-5-9-st-3-5/plug-free-3-5mm-9way/dp/14J3299) |

## Relay outputs subsystem

| Part | Role | Datasheet / source |
|---|---|---|
| Panasonic ALDP105 | SPST-NO relay (x4) | [Panasonic product page](https://na.industrial.panasonic.com/products/relays-contactors/mechanical-power-relays/lineup/general/series/1988/model/136824) · [Future Electronics PDF](https://www1.futureelectronics.com/doc/Panasonic/ALDP105.pdf) |
| Nexperia MMBT3904 | NPN driver transistor (one per relay channel) | [Nexperia datasheet](https://assets.nexperia.com/documents/data-sheet/MMBT3904.pdf) |
| Phoenix Contact MC 1,5/8-ST-3,5 | 8-position (4x COM+NO independent) field connector | [Newark product page](https://www.newark.com/phoenix-contact/mc-1-5-8-st-3-5/pluggable-terminal-block-8-position/dp/14J3298) · [Farnell product page](https://ie.farnell.com/phoenix-contact/mc-1-5-8-st-3-5/terminal-block-pluggable-8pos/dp/5089013) |

## Status indication subsystem

PCA9535PW (I2C GPIO expander) is already listed under "Power subsystem" above — no separate
entry needed, same reuse pattern as LM2904 under "Analog I/O subsystem" below. The 6 status
LEDs are generic 3mm THT parts (red/green/yellow/orange family, chosen for their ~1.8-2.2V
forward voltage — see `docs/subsystems/status-indication.md` Step 4 for why), no specific
manufacturer/part-number match — same "generic, no sourcing risk" treatment as this
project's standard passives.

## Analog I/O subsystem

| Part | Role | Datasheet / source |
|---|---|---|
| TI ADS1115 | 16-bit I2C ADC, isolated side (analog inputs) | [TI datasheet](https://www.ti.com/lit/ds/symlink/ads1115.pdf) |
| TI ISO1540 | I2C digital isolator, both channels bidirectional | [TI datasheet](https://www.ti.com/lit/ds/symlink/iso1541.pdf) (covers both ISO1540/ISO1541 in one document) |
| Microchip MCP4725 | 12-bit I2C DAC, isolated side (analog output) | [Microchip datasheet](https://ww1.microchip.com/downloads/en/devicedoc/22039d.pdf) |
| Littelfuse SMBJ15CA | Bidirectional TVS, analog input overvoltage protection (x2) | [Littelfuse datasheet](https://www.littelfuse.com/assetdocs/tvs-diodes-smbj-series-datasheet) |
| Phoenix Contact MC 1,5/4-ST-3,5 | 4-position field connector (AI1/AI2/AO + shared return) | [Newark product page](https://www.newark.com/phoenix-contact/mc-1-5-4-st-3-5/pluggable-terminal-block-4-position/dp/14J3294) |

The 15V-ANALOG-ISO supply module belongs to `power.kicad_sch`, not this sheet — see Recom
RK-0515S under "Power modules & protection components" above. An earlier candidate for that
rail, Recom R05P215S, is listed there under superseded parts (unresolved isolation-rating
gap, never actually implemented).

LM2904 (input buffers + output gain stage, 2 physical instances) reuses the part already
listed under "Power subsystem" above — no separate entry needed.

## RS485 subsystem

Protection network (GDT/TVS/choke/bias resistors) is Mornsun's own "Fig. 2: Port
protection circuit for harsh environments" from the transceiver's datasheet, not a
generic cascade — see `docs/subsystems/rs485.md` Step 2 for the full reasoning.

| Part | Role | Datasheet / source |
|---|---|---|
| Mornsun TD321S485H-A (TD5(3)21S485H-A series) | Isolated RS485 transceiver module | [Mornsun datasheet](https://www.mornsun-power.com/public/uploads/pdf/TD5(3)21S485H-A.pdf) — full PDF obtained 2026-09-08 (user-supplied): 3kVDC isolation, 500kbps max, built-in 47kΩ A/B pull-down, confirmed 10-pin pinout, and the harsh-environment reference circuit (Fig. 2) this section's other parts come from |
| Bencent B3D090L-C (GD1) | Gas discharge tube (GDT), 3-electrode: line pins wired A-B, third (common) electrode bonds to `EARTH` | [LCSC product page](https://www.lcsc.com/product-detail/Gas-Discharge-Tube-GDT_Bencent-B3D090L-C_C511253.html) — 90V DC spark-over, 5kA @ 8/20µs, 3-pole, 1.5pF. Doubly confirmed: flagged from the original teardown, and Mornsun's own datasheet names the base part "B3D090L" directly |
| Littelfuse SMBJ6.5CA (x3, D15/D18/D19; Mornsun Fig. 2 D1/D2/D3) | Bidirectional TVS diodes — D15 A-B differential clamp, D18/D19 line-to-EARTH clamp | [Littelfuse SMBJ series datasheet](https://www.littelfuse.com/assetdocs/tvs-diodes-smbj-series-datasheet) · [DigiKey](https://www.digikey.com/en/products/detail/littelfuse-inc/SMBJ6-5CA/285958) — 600W, DO-214AA. Mornsun-specified exact part. Same SMBJ family already used for analog-input protection (SMBJ15CA) |
| TDK ACM2520-301-2P (U14) | Common-mode choke (Mornsun Fig. 2 T1) | [TDK product page](https://product.tdk.com/en/search/emc/emc/cmf_cmc/info?part_no=ACM2520-301-2P-T002) · [LCSC](https://www.lcsc.com/product-detail/Common-Mode-Filters_TDK-ACM2520-301-2P-T002_C76577.html) — Mornsun-specified exact part |
| Generic 2.7Ω/2W (R34, R35; Mornsun Fig. 2 R1/R2) | Series current-limiting resistors | Mornsun-specified value/power rating |
| Generic 1MΩ (R36; Mornsun Fig. 2 R3) + 1nF/2kV (C23; Mornsun Fig. 2 C1) | RC snubber to EARTH | Mornsun-specified |
| Generic 4.7kΩ (R_pullup1, R_pulldown1) | External bias — Rpullup: VO (pin 7) to A; Rpulldown: RGND (pin 10) to B | Mornsun specifies only a current ceiling (<25mA) on VO/RGND, not a resistance — 4.7kΩ gives ≈1.1mA worst case, well inside that ceiling; picked and locked on the schematic 2026-09-10 |
| Phoenix Contact MC 1,5/3-ST-3,5 (J5) | 3-position field connector (A, B, EARTH) | [Newark product page](https://www.newark.com/phoenix-contact/mc-1-5-3-st-3-5/pluggable-terminal-block-3-position/dp/14J3293) — MPN 1840379 |

Superseded (2026-09-08, same day — kept here for traceability, not because they were
wrong parts, just replaced by Mornsun's own tested circuit once its datasheet was fully
readable): Bourns CDSOT23-SM712 (TVS), Würth WE-SL2 744227 (common-mode choke), generic
27Ω series resistors.

## RS232 subsystem

No dedicated protection component — decided against an added TVS network; see
`docs/subsystems/rs232.md` decision 3 for the full reasoning (datasheet's own application
circuit has none, receiver is separately rated safe to ±25V unpowered, and reference-design
photo evidence shows no dedicated protection stage for this port, unlike RS485).

| Part | Role | Datasheet / source |
|---|---|---|
| TI MAX3232EIPWR | RS232 transceiver, TSSOP-16, one channel used (DIN1/DOUT1/RIN1/ROUT1); channel 2 left unconnected | [TI MAX3232E datasheet](https://www.ti.com/lit/ds/slls664c/slls664c.pdf) — corrected 2026-09-10 from an earlier placeholder ("MAX3232EI" alone isn't a real SKU); picked over the ADI/Maxim equivalent (MAX3232EEUE+) for SnapEDA symbol/footprint availability |
| Phoenix Contact MC 1,5/3-ST-3,5 | 3-position field connector (TXD, RXD, GND) | [Newark product page](https://www.newark.com/phoenix-contact/mc-1-5-3-st-3-5/pluggable-terminal-block-3-position/dp/14J3293) — MPN 1840379, same part reused from RS485's connector (J5) |

## Connectors & wiring standards

| Item | Role | Source |
|---|---|---|
| Phoenix Contact MC 1,5/2-ST-3,5 | Power input terminal block | [Datasheet](https://www.mouser.com/datasheet/3/507/5/phoenix_contact_1840366_en.pdf) |
| NFPA 70 (NEC) Table 310.15(B)(16) | Wire ampacity reference | Standard reference, consult a current NEC copy |
| Engineering ToolBox AWG current-rating table | Wire ampacity cross-check | https://www.engineeringtoolbox.com/wire-gauges-d_419.html |

## Application notes & reference designs

| Document | Covers | Source |
|---|---|---|
| TI TIDUBP3A — Reverse Battery Protection LM74610-Q1 | Reference design for the reverse-polarity stage: schematic, part selection, rated for 12/24V systems with load-dump survival past 30V | [PDF](https://www.ti.com/lit/ug/tidubp3a/tidubp3a.pdf) |
| TI SLYY127 — Flat-Clamp surge protection technology | Active-feedback clamping mechanism behind TVS3300 | [PDF](https://www.ti.com/lit/slyy127) |
| TVS3300 datasheet, Section 9 | TVS3300's own typical application circuit and layout guidance | [PDF](https://www.ti.com/lit/ds/symlink/tvs3300.pdf) |
| TI SLVA862 — Basics of eFuses | Background on inrush/reverse-polarity/overvoltage protection concepts | ti.com application report SLVA862 |
| WIZnet hardware design guide | Generic decoupling (0.1uF bypass, 10uF/4.7uF bulk, 3.3V regulator >=300mA) | [WIZnet Design Guide](https://docs.wiznet.io/Design-Guide/hardware_design_guide) |
| WIZnet W5500 reference schematic | Transformer/RJ45 config, isolation capacitors | [WIZnet W5500 ref-schematic](https://docs.wiznet.io/Product/Chip/Ethernet/W5500/ref-schematic) |
| Bourns RS-485 Port Protection Evaluation Board design note | GDT + series-limiting + TVS cascade topology reference for RS485 lines (uses a TBU current limiter instead of plain resistors — informed this project's cascade order, not its exact parts) | [PDF](https://www.bourns.com/docs/technical-documents/technical-library/circuit-protection/design-notes/bourns_rs485_evalboard4_design_note.pdf) |
| Würth ANP083 — RS-485 EMI filtering app note | Common-mode choke placement on an RS-485 interface (WE-SL2 family) | Referenced via Würth's WE-SL2 product page; direct PDF link 404'd 2026-09-08, retry before finalizing layout |

No single vendor app note covers this exact combination (wide 10-30V input protection with
multiple MagI3C modules) — the protection sequencing (fuse -> reverse-polarity ->
surge/TVS -> regulation) follows standard practice but isn't cross-checked against one
authoritative reference document.

## Reference open-hardware projects (patterns only, not copied)

| Project | Use |
|---|---|
| [espressif/kicad-libraries](https://github.com/espressif/kicad-libraries) | Official KiCad symbols/footprints |
| [OLIMEX/ESP32-GATEWAY](https://github.com/OLIMEX/ESP32-GATEWAY) | General ESP32 + Ethernet KiCad project structure reference only — its power stage isn't reusable (USB-powered, no field-voltage input) and its license status is unresolved (LICENSE says Apache-2.0, manual says CERN-OHL) |
| [Walter (QuickSpot/DPTechnics)](https://github.com/QuickSpot/walter-documentation) / [walter-arduino](https://github.com/QuickSpot/walter-arduino) | ESP32-S3 <-> cellular modem UART/power-sequencing/antenna pattern, for the LTE subsystem |
| [ElectroSoul-Technologies/ESP32-RS485_Gateway](https://github.com/ElectroSoul-Technologies) | RS485 direction-control + TVS protection reference |

## Not yet re-verified this session

Identified during the original teardown; re-pull the current datasheet from the
manufacturer/distributor before relying on exact figures.

- Würth Elektronik 7499010441 — Ethernet magjack
- Toshiba SSM3J307T family — P-channel MOSFET behind the "PJ307U" board marking

(Bencent B3D090L-C — moved to "RS485 subsystem" above, re-verified 2026-09-08.)

## Not added yet

- DC-DC module and protection component datasheets for subsystems other than power — added
  as each subsystem's own module selection / protection design step is reached.
