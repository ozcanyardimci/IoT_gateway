#include "lte_modem.h"
#include <Arduino.h>
#include "pin_map.h"

bool LteModem::begin() {
    pinMode(PIN_LTE_PWRKEY, OUTPUT);
    digitalWrite(PIN_LTE_PWRKEY, LOW);
    pinMode(PIN_LTE_RESET, OUTPUT);
    digitalWrite(PIN_LTE_RESET, LOW);
    return true;
}

void LteModem::powerOn() {
    digitalWrite(PIN_LTE_PWRKEY, HIGH);
    delay(2200); // datasheet minimum 2000ms + 200ms margin
    digitalWrite(PIN_LTE_PWRKEY, LOW);
}

void LteModem::powerOff() {
    digitalWrite(PIN_LTE_PWRKEY, HIGH);
    delay(3200); // datasheet minimum 3000ms + 200ms margin
    digitalWrite(PIN_LTE_PWRKEY, LOW);
}

void LteModem::hardReset() {
    digitalWrite(PIN_LTE_RESET, HIGH);
    delay(150); // datasheet minimum 100ms + 50ms margin
    digitalWrite(PIN_LTE_RESET, LOW);
}
