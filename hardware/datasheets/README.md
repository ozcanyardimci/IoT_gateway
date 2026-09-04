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

Superseded parts (kept for traceability — see `docs/subsystems/power.md` revision history
for why): Toshiba SSM3J351R,LF (replaced by LM74610-Q1 + CSD18531Q5A), Littelfuse SMBJ36A
(replaced by TVS3300), Recom R1SX-3305 (replaced by R1SX-3.33.3-R — wrong output voltage
for the rail).

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
- Bencent B3D090L-C — gas discharge tube, RS485/RS232 surge protection
- Toshiba SSM3J307T family — P-channel MOSFET behind the "PJ307U" board marking

## Not added yet

- DC-DC module and protection component datasheets for subsystems other than power — added
  as each subsystem's own module selection / protection design step is reached.
