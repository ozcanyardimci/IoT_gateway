#include "pca9535.h"
#include <Wire.h>

#define REG_INPUT_PORT0  0x00
#define REG_OUTPUT_PORT0 0x02
#define REG_CONFIG_PORT0 0x06

static bool write_reg16(uint8_t addr, uint8_t reg, uint16_t value) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write((uint8_t)(value & 0xFF));        // port 0 (low byte)
    Wire.write((uint8_t)((value >> 8) & 0xFF)); // port 1 (high byte), auto-increment
    return Wire.endTransmission() == 0;
}

static bool read_reg16(uint8_t addr, uint8_t reg, uint16_t *out) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false; // repeated start
    if (Wire.requestFrom((int)addr, 2) != 2) return false;
    uint8_t lo = (uint8_t)Wire.read();
    uint8_t hi = (uint8_t)Wire.read();
    *out = (uint16_t)lo | ((uint16_t)hi << 8);
    return true;
}

PCA9535::PCA9535(uint8_t i2c_address) : addr_(i2c_address) {}

bool PCA9535::begin() {
    Wire.beginTransmission(addr_);
    if (Wire.endTransmission() != 0) return false;
    return readOutputs(&output_shadow_);
}

bool PCA9535::setPortDirection(uint16_t dir_mask) {
    return write_reg16(addr_, REG_CONFIG_PORT0, dir_mask);
}

bool PCA9535::writeOutputs(uint16_t value) {
    if (!write_reg16(addr_, REG_OUTPUT_PORT0, value)) return false;
    output_shadow_ = value;
    return true;
}

bool PCA9535::readOutputs(uint16_t *out_value) const {
    return read_reg16(addr_, REG_OUTPUT_PORT0, out_value);
}

bool PCA9535::readInputs(uint16_t *out_value) const {
    return read_reg16(addr_, REG_INPUT_PORT0, out_value);
}

bool PCA9535::writePin(int pin, bool level) {
    if (pin < 0 || pin > 15) return false;
    uint16_t next = output_shadow_;
    if (level) next = (uint16_t)(next | (1u << pin));
    else       next = (uint16_t)(next & ~(1u << pin));
    return writeOutputs(next);
}
