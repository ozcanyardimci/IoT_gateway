#pragma once
#include <HardwareSerial.h>

// One isolated half-duplex RS485 bus, TXD=GPIO1, RXD=GPIO2 (pin_map.h).
// The isolated transceiver module appears to have automatic direction
// sensing - no DE/RE control signal appears anywhere in
// architecture.md's pin table (confirmed by its absence, not just left
// unassigned - re-check docs/subsystems/rs485.md's connector pinout if
// that turns out wrong). This driver is a plain UART wrapper; the module
// handles bus turnaround itself.
//
// Uses ESP32 HardwareSerial instance 1 (instance 0 is reserved - it's the
// module's default UART0, shared with RELAY3/4_CTRL per architecture.md;
// instance 2 is used by RS232, see rs232_serial.h). Default baud 9600 -
// this project's docs establish a 150kbit/s guaranteed-minimum ceiling on
// the line driver, not a target operating rate; set it to whatever the
// actual field devices need.
class Rs485Serial {
public:
    void begin(unsigned long baud = 9600);
    HardwareSerial &port();

private:
    HardwareSerial serial_{1};
};
