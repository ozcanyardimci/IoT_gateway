#include "lte_ppp.h"
#include <PPP.h>
#include "pin_map.h"

namespace {
// NOT PPPClass::begin()'s default (1) - see lte_ppp.h's header comment
// on why 1 would collide with Rs485Serial.
const uint8_t kLtePppUartNum = 0;
const int kLtePppBaud = 115200; // matches this modem's documented
                                 // power-on default baud, same value
                                 // lte_modem used before this module
                                 // took over the UART
} // namespace

bool LtePpp::begin(const char *apn, const char *sim_pin) {
    if (apn == nullptr || apn[0] == '\0') {
        return false;
    }

    PPP.setPins(PIN_LTE_TXD, PIN_LTE_RXD);
    PPP.setApn(apn);
    if (sim_pin != nullptr && sim_pin[0] != '\0') {
        PPP.setPin(sim_pin);
    }

    return PPP.begin(PPP_MODEM_GENERIC, kLtePppUartNum, kLtePppBaud);
}

bool LtePpp::isAttached() const {
    return PPP.attached();
}

bool LtePpp::switchToDataMode() {
    return PPP.mode(ESP_MODEM_MODE_CMUX);
}

bool LtePpp::isConnected() const {
    return PPP.connected();
}

int16_t LtePpp::signalQualityDbm() const {
    return (int16_t)PPP.RSSI();
}
