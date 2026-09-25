#pragma once
#include <stdint.h>

// Generic driver for the NXP PCA9535 16-bit I2C GPIO expander - register
// map per NXP's datasheet (Input/Output/Polarity/Configuration, 2 ports of
// 8 bits, auto-increment addressing). Not specific to this project's LED
// usage - see status_leds.h for that.
class PCA9535 {
public:
    explicit PCA9535(uint8_t i2c_address);

    // Probes the device (address-only transmission). Returns true if it ACKs.
    bool begin();

    // dir_mask bit=1 -> input, bit=0 -> output (matches the chip's own
    // convention and its power-on default of 0xFFFF = all input).
    bool setPortDirection(uint16_t dir_mask);

    bool writeOutputs(uint16_t value);
    bool readOutputs(uint16_t *out_value) const;

    // Reads actual pin states (inputs and outputs both read back here).
    bool readInputs(uint16_t *out_value) const;

    // pin is 0-15 (IO0_0..IO0_7 = 0-7, IO1_0..IO1_7 = 8-15, matching the
    // datasheet's own numbering). Only meaningful for pins set as outputs.
    bool writePin(int pin, bool level);

private:
    uint8_t addr_;
    uint16_t output_shadow_ = 0x0000; // read-modify-write cache for writePin()
};
