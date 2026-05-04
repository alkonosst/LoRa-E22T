/**
 * SPDX-FileCopyrightText: 2026 Maximiliano Ramirez <maximiliano.ramirezbravo@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 */

/** Explanation of this example:
 * - This example demonstrates how to read the ambient RF noise level (RSSI) from the module.
 * - It enables the ambient RSSI feature, then polls the module every second and prints the result.
 * - Useful for surveying the RF environment before deploying a network.
 * - Use the Serial Monitor to send the following commands:
 *   - `s`: Start polling ambient RSSI every second.
 *   - `x`: Stop polling.
 */

#include <Arduino.h>

#include <LoRa-E22T.h>
using namespace E22;

// Pins
static constexpr uint8_t PIN_E22_M0    = 9;
static constexpr uint8_t PIN_E22_M1    = 10;
static constexpr uint8_t PIN_E22_AUX   = 13;
static constexpr int8_t PIN_E22_RESET  = -1; // not used in this example
static constexpr uint8_t PIN_SERIAL_TX = 11;
static constexpr uint8_t PIN_SERIAL_RX = 12;

// Module config
static constexpr Model MODULE_MODEL   = Model::E22_900T30;
static constexpr uint32_t MODULE_BAUD = 9600;

// Instance
static LoRaE22T lora;

void setup() {
  Serial.begin(115200);
  Serial1.begin(MODULE_BAUD, SERIAL_8N1, PIN_SERIAL_RX, PIN_SERIAL_TX);
  delay(2000);

  Serial.println("-------------------------------");
  Serial.println("LoRa-E22T - AmbientRSSI Example");
  Serial.println("-------------------------------");

  Status status = lora.begin(MODULE_MODEL, Serial1, PIN_E22_M0, PIN_E22_M1, PIN_E22_AUX);
  if (status != Status::Ok) {
    Serial.printf("Failed to initialize module: %s\r\n", statusToString(status));
    while (true)
      ;
  }

  status = lora.setFactorySettings();
  if (status != Status::Ok) {
    Serial.printf("Failed to set factory settings: %s\r\n", statusToString(status));
    while (true)
      ;
  }

  // Enable ambient RSSI feature (requires Configuration mode)
  status = lora.setMode(Mode::Configuration);
  if (status != Status::Ok) {
    Serial.printf("Failed to enter configuration mode: %s\r\n", statusToString(status));
    while (true)
      ;
  }

  status = lora.setAmbientRSSI(/*enable*/ true, /*persistence*/ false);
  if (status != Status::Ok) {
    Serial.printf("Failed to enable ambient RSSI: %s\r\n", statusToString(status));
    while (true)
      ;
  }

  status = lora.setMode(Mode::Transmission);
  if (status != Status::Ok) {
    Serial.printf("Failed to enter transmission mode: %s\r\n", statusToString(status));
    while (true)
      ;
  }

  Serial.println("Module initialized successfully!\r\n");
  Serial.println("Available commands:");
  Serial.println("> 's': Start polling ambient RSSI");
  Serial.println("> 'x': Stop polling\r\n");
}

void loop() {
  static bool polling          = false;
  static uint32_t last_poll_ms = 0;

  if (Serial.available()) {
    char c = Serial.read();
    if (c == '\r') return;

    if (c == '\n') {
      Serial.println(">");
    } else {
      Serial.printf("> %c\r\n", c);
    }

    switch (c) {
      case 's':
      {
        polling      = true;
        last_poll_ms = 0;
        Serial.println("Polling started.");
      } break;

      case 'x':
      {
        polling = false;
        Serial.println("Polling stopped.");
      } break;
    }
  }

  if (!polling) return;

  if (millis() - last_poll_ms < 1000) return;
  last_poll_ms = millis();

  int16_t rssi_dbm = 0;
  Status s         = lora.readAmbientRSSI(rssi_dbm);
  if (s != Status::Ok) {
    Serial.printf("readAmbientRSSI failed: %s\r\n", statusToString(s));
    return;
  }

  Serial.printf("Ambient RSSI: %d dBm\r\n", rssi_dbm);
}
