#pragma once
#include "pca9535.h"

// Wraps the PCA9535 expander with the 6 status LEDs this project defines
// (docs/subsystems/status-indication.md): PWR-OK, HEARTBEAT, LTE, WIFI,
// ETH, FAULT.
//
// CONFIRMED from the docs: I2C address 0x20 (pin_map.h), and the LEDs are
// active-LOW - the doc's resistor math is keyed to the PCA9535's
// guaranteed IOL *sink* current, meaning the expander pin sinks current
// through the LED to light it (drive the pin LOW to turn the LED on).
//
// NOT CONFIRMED: which of the 10 spare IO0_x/IO1_x pins each of the 6 LEDs
// actually uses - the docs name the 6 LEDs and say 6-of-16 pins are used,
// but not the exact pin assignment. LED_PIN[] in the .cpp is a placeholder
// (first 6 pins in datasheet order) - confirm the real mapping against
// status_indication.kicad_sch and fix that one array; nothing else here
// depends on which placeholder values are there now.
enum class StatusLed {
    PWR_OK = 0,
    HEARTBEAT = 1,
    LTE = 2,
    WIFI = 3,
    ETH = 4,
    FAULT = 5,
};

class StatusLeds {
public:
    StatusLeds();
    bool begin();
    bool set(StatusLed led, bool on);

private:
    PCA9535 expander_;
};
