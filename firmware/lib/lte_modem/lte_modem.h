#pragma once
#include <stdint.h>
#include <stddef.h>

// Quectel EG915U-EU LTE modem: POWER SEQUENCING ONLY. PWRKEY=GPIO15,
// RESET=GPIO16, both via NPN low-side driver transistors.
//
// Power sequencing polarity CONFIRMED 2026-09-23 by three independent
// checks, not assumed:
// (1) Quectel's own EG915U Hardware Design doc: PWRKEY and RESET_N are
//     both active-low AT THE MODEM PIN ("VBAT power domain. Active low.").
// (2) The actual KiCad schematic (lte.kicad_sch, read directly - not just
//     the subsystem doc's prose): base series resistor 4.7k + base-
//     emitter pulldown 47k + emitter->GND_LOGIC + collector->PWRKEY/
//     RESET_N directly - a standard NPN low-side switch. (Side finding:
//     the schematic's real reference designators are R49/R50/R51/R52 and
//     Q6/Q7 - docs/subsystems/lte.md still cites an older R33-R36/Q4-Q5
//     numbering from before a renumbering pass. Same circuit, stale
//     labels in that doc - worth a fix there, not a functional issue.)
// (3) Circuit theory for that topology: GPIO HIGH -> transistor saturates
//     -> collector pulled to ~GND -> PWRKEY/RESET_N asserted (active-low
//     at the modem). GPIO LOW/floating -> transistor off -> the modem's
//     own internal pull-up (no external pull-up populated, per lte.md)
//     holds the pin inactive-high.
// So: MCU GPIO HIGH = asserted, MCU GPIO LOW = idle. Implemented below.
//
// TRIMMED 2026-09-24: this class used to also own the modem's UART (AT
// command transport, AT+CSQ/AT+CREG polling - see git history / this
// project's docs/roadmap.md F4 entry for what it used to do) alongside
// power control. That UART ownership moved to the new lib/lte_ppp
// module, which needs EXCLUSIVE control of the same UART0 peripheral
// (same PIN_LTE_TXD/PIN_LTE_RXD pins) for arduino-esp32's PPP library
// to frame AT/PPP traffic correctly - two independent HardwareSerial
// users on the same UART would corrupt each other's reads. This class
// now does ONLY power/reset GPIO sequencing (which lte_ppp does NOT
// duplicate - see lte_ppp.h's own header comment for why that split is
// deliberate, not an oversight). lte_at_parse.h/.c (AT+CSQ/AT+CREG
// response parsing) was deleted as part of the same change - it had no
// other dependents (confirmed via repo-wide grep before deleting) once
// this class stopped calling sendAt() itself.
//
// Caller sequencing requirement: begin() + powerOn() here MUST run
// BEFORE lte_ppp's begin() - the modem needs its power-on boot time
// (datasheet: several seconds after PWRKEY release before it responds
// to AT commands) before PPP.begin() starts talking to it. This class
// does not enforce that ordering itself; main.cpp's setup() does, by
// calling them in that order.
class LteModem {
public:
    // Configures PWRKEY/RESET as outputs, idle (LOW). Does NOT touch
    // any UART - that's lte_ppp's job now.
    bool begin();

    // Pulses PWRKEY for the datasheet-minimum duration (+ margin) to power
    // the module on or off.
    void powerOn();
    void powerOff();

    // Pulses RESET for the datasheet-minimum duration (+ margin) - a
    // baseband reset; the module stays powered.
    void hardReset();
};
