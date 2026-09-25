#pragma once
#include <stdint.h>

// 4 relay outputs via an NPN transistor driver stage. Confirmed
// active-HIGH at the GPIO (docs/subsystems/relay-outputs.md acceptance
// criteria): GPIO HIGH energizes the coil; GPIO LOW or floating holds it
// off (board has a pull-down, so it's also safe before firmware sets pin
// modes at boot).
class RelayOutputs {
public:
    static const int CHANNEL_COUNT = 4;

    // De-energizes all relays. Call once at startup.
    void begin();

    bool setChannel(int index, bool energized);
    bool getChannel(int index) const;

    // Safety helper for fault/shutdown paths.
    void allOff();

private:
    static const uint8_t PINS[CHANNEL_COUNT];
    bool state_[CHANNEL_COUNT] = {false, false, false, false};
};
