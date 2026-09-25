#pragma once
#include <stdint.h>

// Microchip MCP4725 12-bit I2C DAC (docs/subsystems/analog-io.md) - the
// board's single analog output channel. Uses Fast Mode Write (volatile
// only, no EEPROM write) - resets to 0 on power loss, the safer default
// for an actuator output.
class McpAnalogOutput {
public:
    bool begin();

    // value12bit: 0-4095, clamped if out of range. Returns false on I2C error.
    bool writeRaw(uint16_t value12bit);
};
