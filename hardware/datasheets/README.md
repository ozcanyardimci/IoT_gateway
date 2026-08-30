# Datasheets, sources & application notes

Links only — no PDF binaries are stored in this repo. Reasons: repo size stays manageable,
official documents get updated by the manufacturer over time (a stored local copy goes
stale), and redistributing third-party copyrighted PDFs in a repo isn't something
manufacturers actually license, even informally. Standard practice is to link the
authoritative source and re-pull it when needed. This file is a living document — updated
as each subsystem is worked through, not filled in all at once.

## Power subsystem (verified 2026-08-30)

| Part | Role | Datasheet / source |
|---|---|---|
| ESP32-S3-WROOM-1U | Main MCU module | [Espressif official datasheet](https://documentation.espressif.com/api/resource/doc/file/3yD6w5Y5/FILE/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf) |
| Quectel EG915U-EU | LTE Cat-1 modem | [Quectel EG915U Series Hardware Design v1.1](https://quectel.com/content/uploads/2024/02/Quectel_EG915U_Series_Hardware_Design_V1.1.pdf) |
| WIZnet W5500 | SPI Ethernet controller | [WIZnet official datasheet v1.1.0](https://docs.wiznet.io/img/products/w5500/W5500_ds_v110e.pdf) |
| NXP PCA9535PW | I2C GPIO expander | [NXP datasheet](https://www.nxp.com/docs/en/data-sheet/PCA9535_PCA9535C.pdf) · [TI second-source cross-check](https://www.ti.com/lit/ds/symlink/pca9535.pdf) |
| MAX3232E | RS232 transceiver | [Analog Devices/Maxim MAX3222E-MAX3246E family datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/max3222e-max3246e.pdf) |
| Mornsun TD321S485H-A | Isolated RS485 module | [Mornsun datasheet](https://www.mornsun-power.com/public/uploads/pdf/TD5(3)21S485H-A.pdf) |
| Panasonic ALDP105 | Relay (4x) | [Panasonic Industrial product page](https://na.industrial.panasonic.com/products/relays-contactors/mechanical-power-relays/lineup/general/series/1988/model/136824) · [Future Electronics PDF cross-check](https://www1.futureelectronics.com/doc/Panasonic/ALDP105.pdf) |
| Lite-On LTV-247 | Digital-input optocoupler | [Datasheet via LCSC](https://datasheet.lcsc.com/datasheet/pdf/6ec3b014c2c1b1e2589a5b88a431fe52.pdf?productCode=C115451) |
| AME8805/8813 | Same-family reference for AME8808 (analog LDO) — **AME8808's own datasheet not found in extractable form, treat as unverified substitute, not confirmed** | [AME official datasheet](https://www.ame.com.tw/datasheet/11-AME8805%208813_061313_R.13.pdf) |
| LM2904 | Analog stage op-amp | [Diodes Inc. LM2902/LM2904 datasheet](https://www.diodes.com/assets/Datasheets/LM2902-04.pdf) |

## Application notes (distinct from plain datasheets — layout/decoupling guidance)

| Document | Covers | Source |
|---|---|---|
| ESP32-S3 Hardware Design Guidelines | Power-supply decoupling: 3.3V/>=500mA supply, 0.1uF caps near digital power pins, 0.1uF+1uF near VDD_SPI, 10uF on VDD3P3 (analog), >=10uF + ESD diode at main power entrance | [Espressif docs](https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32s3/index.html) ([PDF](https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32s3/esp-hardware-design-guidelines-en-master-esp32s3.pdf)) |
| WIZnet hardware design guide | Generic decoupling (0.1uF bypass, 10uF/4.7uF bulk, 3.3V regulator >=300mA) | [WIZnet Design Guide](https://docs.wiznet.io/Design-Guide/hardware_design_guide) |
| WIZnet W5500 reference schematic | Transformer/RJ45 config, isolation capacitors — W5500-specific detail lives in the linked schematic PDFs, not a single unified app note | [WIZnet W5500 ref-schematic](https://docs.wiznet.io/Product/Chip/Ethernet/W5500/ref-schematic) |

## Reference open-hardware projects (not copied — used as cross-checks / patterns only)

| Project | Use |
|---|---|
| [espressif/kicad-libraries](https://github.com/espressif/kicad-libraries) | Official KiCad symbols/footprints — import before schematic capture |
| [OLIMEX/ESP32-GATEWAY](https://github.com/OLIMEX/ESP32-GATEWAY) | Reviewed for power-stage reuse 2026-08-30 — **not usable**: USB-powered, single 5V→3.3V buck, no field-voltage input or high-current transient load, and its own license status is unresolved (LICENSE file says Apache-2.0, current user manual says CERN-OHL). Kept only as a general "ESP32 + Ethernet KiCad project structure" reference. |
| [Walter (QuickSpot/DPTechnics)](https://github.com/QuickSpot/walter-documentation) / [walter-arduino](https://github.com/QuickSpot/walter-arduino) | ESP32-S3 ↔ cellular modem UART/power-sequencing/antenna pattern reference — for LTE subsystem |
| [ElectroSoul-Technologies/ESP32-RS485_Gateway](https://github.com/ElectroSoul-Technologies) | RS485 direction-control + TVS protection reference — for RS485 subsystem |

## Not yet re-verified this session (from earlier teardown identification — locate current
## datasheet via manufacturer/distributor before relying on exact figures)
- Würth Elektronik 7499010441 — Ethernet magjack
- Bencent B3D090L-C — gas discharge tube, RS485/RS232 surge protection
- Toshiba SSM3J307T family — P-channel MOSFET behind the "PJ307U" marking, reverse-polarity protection

## Deliberately not here yet (sequencing, not an oversight)
- **DC-DC module datasheets** (the actual 3.3V/5V/LTE-rail regulator parts) — not added yet
  because module selection (power roadmap step 4) hasn't happened. Will be added once chosen.
- **Protection component datasheets** (fuse/PTC, TVS, Y-cap part numbers) — not added yet
  because protection design (power roadmap step 5) hasn't happened.
- This file grows per subsystem as work reaches each one — it will not be "complete" until
  the whole project is. Treat gaps above this note as pending, and anything silently missing
  as worth flagging.
