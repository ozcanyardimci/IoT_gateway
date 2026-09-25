#pragma once
#include <stdint.h>

// 8 opto-isolated digital inputs. Logic sense is ACTIVE-LOW at the GPIO:
// field device active -> opto LED on -> phototransistor conducts -> node
// pulled low (confirmed: docs/subsystems/digital-inputs.md, "Logic sense").
// read_channel() returns the field-logical state (true = field input
// active) - already inverted, callers don't handle polarity.
//
// An external 10k pull-up to 3.3V-LOGIC is already on the board (that same
// doc's resistor sizing) - pinMode is plain INPUT, not INPUT_PULLUP.
class DigitalInputs {
public:
    static const int CHANNEL_COUNT = 8;

    void begin();

    // index 0-7 maps to DI1-DI8. Returns false (inactive) if index is out
    // of range - callers doing a fixed 0..7 loop never hit this.
    bool readChannel(int index) const;

    // Reads all 8 channels into out[0..7] (field-logical sense).
    void readAll(bool out[CHANNEL_COUNT]) const;

private:
    static const uint8_t PINS[CHANNEL_COUNT];
};
