#include "ads1115_adc.h"
#include <Arduino.h>
#include <Wire.h>
#include "pin_map.h"

#define ADS1115_REG_CONVERSION 0x00
#define ADS1115_REG_CONFIG     0x01

// MUX select for single-ended AIN0/AIN1 (TI datasheet, Config register Table).
#define ADS1115_MUX_AIN0 0x4u // 100
#define ADS1115_MUX_AIN1 0x5u // 101

bool AdsAnalogInput::begin() {
    Wire.beginTransmission(I2C_ADDR_ADS1115);
    return Wire.endTransmission() == 0;
}

bool AdsAnalogInput::startConversion(uint8_t mux_select) {
    // OS=1 (start), MUX=mux_select, PGA=010 (+-2.048V), MODE=1 (single-shot),
    // DR=100 (128SPS), COMP_QUE=11 (comparator disabled - ALERT/RDY unused).
    uint16_t config = 0;
    config = (uint16_t)(config | (1u << 15));
    config = (uint16_t)(config | ((mux_select & 0x7u) << 12));
    config = (uint16_t)(config | (0x2u << 9));
    config = (uint16_t)(config | (1u << 8));
    config = (uint16_t)(config | (0x4u << 5));
    config = (uint16_t)(config | 0x3u);

    Wire.beginTransmission(I2C_ADDR_ADS1115);
    Wire.write(ADS1115_REG_CONFIG);
    Wire.write((uint8_t)(config >> 8));
    Wire.write((uint8_t)(config & 0xFF));
    return Wire.endTransmission() == 0;
}

bool AdsAnalogInput::waitForConversion() {
    // Poll OS (bit 15 of the config register; 1 = conversion complete)
    // rather than a fixed delay - robust if the data rate ever changes.
    // ~50ms timeout as a safety net against a stuck bus.
    for (int attempts = 0; attempts < 50; attempts++) {
        Wire.beginTransmission(I2C_ADDR_ADS1115);
        Wire.write(ADS1115_REG_CONFIG);
        if (Wire.endTransmission(false) != 0) return false;
        if (Wire.requestFrom((int)I2C_ADDR_ADS1115, 2) != 2) return false;
        uint8_t hi = (uint8_t)Wire.read();
        Wire.read(); // lo byte, unused here
        if (hi & 0x80) return true;
        delay(1);
    }
    return false;
}

bool AdsAnalogInput::readConversionResult(int16_t *out_raw) {
    Wire.beginTransmission(I2C_ADDR_ADS1115);
    Wire.write(ADS1115_REG_CONVERSION);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom((int)I2C_ADDR_ADS1115, 2) != 2) return false;
    uint8_t hi = (uint8_t)Wire.read();
    uint8_t lo = (uint8_t)Wire.read();
    *out_raw = (int16_t)(((uint16_t)hi << 8) | lo);
    return true;
}

bool AdsAnalogInput::readChannel(int index, int16_t *out_raw) {
    if (out_raw == nullptr) return false;
    uint8_t mux;
    if (index == 0) mux = ADS1115_MUX_AIN0;
    else if (index == 1) mux = ADS1115_MUX_AIN1;
    else return false;

    if (!startConversion(mux)) return false;
    if (!waitForConversion()) return false;
    return readConversionResult(out_raw);
}
