#include "di_driver.h"
#include <Arduino.h>
#include "pin_map.h"

const uint8_t DigitalInputs::PINS[DigitalInputs::CHANNEL_COUNT] = {
    PIN_DI1, PIN_DI2, PIN_DI3, PIN_DI4, PIN_DI5, PIN_DI6, PIN_DI7, PIN_DI8
};

void DigitalInputs::begin() {
    for (int i = 0; i < CHANNEL_COUNT; i++) {
        pinMode(PINS[i], INPUT);
    }
}

bool DigitalInputs::readChannel(int index) const {
    if (index < 0 || index >= CHANNEL_COUNT) return false;
    return digitalRead(PINS[index]) == LOW; // active-low
}

void DigitalInputs::readAll(bool out[CHANNEL_COUNT]) const {
    for (int i = 0; i < CHANNEL_COUNT; i++) {
        out[i] = readChannel(i);
    }
}
