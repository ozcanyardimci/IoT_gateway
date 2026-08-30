#include <Arduino.h>

// Toolchain sanity check.
// Purpose: confirm the PlatformIO + pioarduino + ESP32-S3 build pipeline works,
// end to end, before any real subsystem firmware is written.
//
// No physical ESP32-S3 devkit is owned yet, so this runs in the Wokwi simulator
// (see wokwi.toml + diagram.json in this folder) instead of real hardware.
// GPIO21 drives a simulated LED wired in diagram.json.

#define SANITY_LED_PIN 21

void setup() {
  Serial.begin(115200);
  pinMode(SANITY_LED_PIN, OUTPUT);
}

void loop() {
  digitalWrite(SANITY_LED_PIN, HIGH);
  Serial.println("IoT_gateway toolchain check: alive");
  delay(500);
  digitalWrite(SANITY_LED_PIN, LOW);
  delay(500);
}
