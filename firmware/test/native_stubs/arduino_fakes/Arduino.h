#pragma once
#include <stdint.h>
#include "HardwareSerial.h"

// Shared native-test fake for Arduino.h. Declares (does not define) the
// three free functions modbus_master.cpp and other library sources call
// (millis/micros/delay), plus the global Serial1 instance every
// HardwareSerial consumer expects to exist - matching the real
// arduino-esp32 core's own globals. Bodies are defined per-test (in
// whichever test_*.cpp actually needs specific timing behavior), with
// matching extern "C" linkage, so this header can be shared across every
// native test without forcing one fixed implementation on all of them.

extern HardwareSerial Serial1;

#ifdef __cplusplus
extern "C" {
#endif
unsigned long millis();
unsigned long micros();
void delay(uint32_t ms);
#ifdef __cplusplus
}
#endif
