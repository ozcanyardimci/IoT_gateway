#include "status_leds.h"
#include "pin_map.h"

// PLACEHOLDER - see status_leds.h header comment.
static const int LED_PIN[6] = {0, 1, 2, 3, 4, 5}; // IO0_0..IO0_5

StatusLeds::StatusLeds() : expander_(I2C_ADDR_PCA9535) {}

bool StatusLeds::begin() {
    if (!expander_.begin()) return false;
    uint16_t dir_mask = 0xFFFF;
    for (int i = 0; i < 6; i++) {
        dir_mask = (uint16_t)(dir_mask & ~(1u << LED_PIN[i])); // 6 LED pins -> output
    }
    if (!expander_.setPortDirection(dir_mask)) return false;
    return expander_.writeOutputs(0xFFFF); // active-low: all HIGH = all off
}

bool StatusLeds::set(StatusLed led, bool on) {
    int idx = (int)led;
    if (idx < 0 || idx >= 6) return false;
    return expander_.writePin(LED_PIN[idx], !on); // active-low
}
