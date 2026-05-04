/**
 * SPDX-FileCopyrightText: 2026 Maximiliano Ramirez <maximiliano.ramirezbravo@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 */

/** Explanation of this example:
 * - This example demonstrates how to send data in transparent mode.
 * - In transparent mode, all modules with the same address and channel receive the transmission.
 * - Pair this example with TransparentReceive running on a second module.
 * - Use the Serial Monitor to send the following commands:
 *   - `s`: Send a text message to all modules on the same address and channel.
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

  Serial.println("-----------------------------------");
  Serial.println("LoRa-E22T - TransparentSend Example");
  Serial.println("-----------------------------------");

  Status status = lora.begin(MODULE_MODEL, Serial1, PIN_E22_M0, PIN_E22_M1, PIN_E22_AUX);
  if (status != Status::Ok) {
    Serial.printf("Failed to initialize module: %s\r\n", statusToString(status));
    while (true)
      ;
  }

  // Ensure the module is in transparent mode
  status = lora.setMode(Mode::Configuration);
  if (status != Status::Ok) {
    Serial.printf("Failed to enter configuration mode: %s\r\n", statusToString(status));
    while (true)
      ;
  }

  status = lora.setTransmissionMode(TxMode::Transparent, false);
  if (status != Status::Ok) {
    Serial.printf("Failed to set transparent mode: %s\r\n", statusToString(status));
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
  Serial.println("> 's': Send a message\r\n");
}

void loop() {
  if (!Serial.available()) return;

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
      Serial.println("Sending transparent message...");

      static uint32_t send_count = 0;
      static char msg[32];
      snprintf(msg, sizeof(msg), "Hello from sender! #%lu", ++send_count);

      Status s = lora.sendTransparentData(reinterpret_cast<const uint8_t*>(msg), strlen(msg));
      if (s != Status::Ok) {
        Serial.printf("[msg #%lu] Send failed: %s\r\n", send_count, statusToString(s));
      } else {
        Serial.printf("[msg #%lu] Sent: \"%s\"\r\n", send_count, msg);
      }
    } break;
  }
}
