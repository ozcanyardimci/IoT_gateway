#include <Arduino.h>

// Toolchain sanity check.
// Purpose: confirm the PlatformIO + pioarduino + ESP32-S3 build/upload/monitor
// pipeline actually works before any real subsystem firmware is written.
// Not part of the gateway's real functionality.

#ifndef LED_BUILTIN
#define LED_BUILTIN 2  // adjust to your devkit's actual onboard LED pin if different
#endif

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("IoT_gateway toolchain check: alive");
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
}
