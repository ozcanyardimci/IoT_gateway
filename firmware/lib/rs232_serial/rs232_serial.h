#pragma once
#include <HardwareSerial.h>

// RS232 (TI MAX3232EIPWR), TXD=GPIO5, RXD=GPIO6 (pin_map.h), full duplex,
// no DE/RE (confirmed: docs/subsystems/rs232.md, "No DE/RE - that's an
// RS485-specific...signal"). Default baud 9600 - docs establish a
// 150kbit/s guaranteed-minimum ceiling on the line driver, not a target
// rate; adjust to what the connected equipment needs.
//
// Originally scoped with IEC 62056-21 meter reading in mind, which is
// currently deferred (docs/roadmap.md Firmware roadmap, F3 "Deferred").
// This driver is transport-only - no meter protocol layered on top yet.
class Rs232Serial {
public:
    void begin(unsigned long baud = 9600);
    HardwareSerial &port();

private:
    HardwareSerial serial_{2};
};
