#include "mcp4725_dac.h"
#include <Wire.h>
#include "pin_map.h"

bool McpAnalogOutput::begin() {
    Wire.beginTransmission(I2C_ADDR_MCP4725);
    return Wire.endTransmission() == 0;
}

bool McpAnalogOutput::writeRaw(uint16_t value12bit) {
    if (value12bit > 0x0FFF) value12bit = 0x0FFF;
    // Fast Mode Write: C2C1=00, PD1PD0=00 (normal operation), D11-D8 in the
    // low nibble of byte 1, D7-D0 in byte 2.
    Wire.beginTransmission(I2C_ADDR_MCP4725);
    Wire.write((uint8_t)((value12bit >> 8) & 0x0F));
    Wire.write((uint8_t)(value12bit & 0xFF));
    return Wire.endTransmission() == 0;
}
