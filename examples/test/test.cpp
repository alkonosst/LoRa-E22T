#include <Arduino.h>

#include <LoRa-E22T.h>

void setup() {
  Serial.begin(115200);
  delay(2000);
}

void loop() {
  Serial.println("Hello, world!");
  delay(1000);
}