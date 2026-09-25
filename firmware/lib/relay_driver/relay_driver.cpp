#include "relay_driver.h"
#include <Arduino.h>
#include "pin_map.h"

const uint8_t RelayOutputs::PINS[RelayOutputs::CHANNEL_COUNT] = {
    PIN_RELAY1, PIN_RELAY2, PIN_RELAY3, PIN_RELAY4
};

void RelayOutputs::begin() {
    for (int i = 0; i < CHANNEL_COUNT; i++) {
        pinMode(PINS[i], OUTPUT);
        digitalWrite(PINS[i], LOW);
        state_[i] = false;
    }
}

bool RelayOutputs::setChannel(int index, bool energized) {
    if (index < 0 || index >= CHANNEL_COUNT) return false;
    digitalWrite(PINS[index], energized ? HIGH : LOW);
    state_[index] = energized;
    return true;
}

bool RelayOutputs::getChannel(int index) const {
    if (index < 0 || index >= CHANNEL_COUNT) return false;
    return state_[index];
}

void RelayOutputs::allOff() {
    for (int i = 0; i < CHANNEL_COUNT; i++) {
        setChannel(i, false);
    }
}
