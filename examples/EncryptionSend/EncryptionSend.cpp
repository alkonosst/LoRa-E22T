/**
 * SPDX-FileCopyrightText: 2026 Maximiliano Ramirez <maximiliano.ramirezbravo@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 */

/** Explanation of this example:
 * - This example demonstrates how to configure a 16-bit encryption key on the module.
 * - Both sender and receiver must share the same key for communication to succeed. A module with a
 *   different key (or key 0x0000) will not be able to decode the packets.
 * - Note: the key is write-only on the module; reading it back always returns 0x0000.
 * - Pair this example with EncryptionReceive configured with the same ENCRYPTION_KEY.
 * - Use the Serial Monitor to send the following commands:
 *   - `s`: Send an encrypted message to the receiver.
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

// Encryption key: must match on both sender and receiver. Set to 0x0000 to disable.
static constexpr uint16_t ENCRYPTION_KEY = 0xABCD;

// Destination address and channel (fixed mode)
static constexpr uint16_t DEST_ADDRESS = 0x0001;
static constexpr uint8_t DEST_CHANNEL  = 0x12; // channel 18 = 868.125 MHz for 900 MHz models

// Instance
static LoRaE22T lora;

void setup() {
  Serial.begin(115200);
  Serial1.begin(MODULE_BAUD, SERIAL_8N1, PIN_SERIAL_RX, PIN_SERIAL_TX);
  delay(2000);

  Serial.println("----------------------------------");
  Serial.println("LoRa-E22T - EncryptionSend Example");
  Serial.println("----------------------------------");

  Status status = lora.begin(MODULE_MODEL, Serial1, PIN_E22_M0, PIN_E22_M1, PIN_E22_AUX);
  if (status != Status::Ok) {
    Serial.printf("Failed to initialize module: %s\r\n", statusToString(status));
    while (true)
      ;
  }

  status = lora.setMode(Mode::Configuration);
  if (status != Status::Ok) {
    Serial.printf("Failed to enter configuration mode: %s\r\n", statusToString(status));
    while (true)
      ;
  }

  status = lora.setFactorySettings();
  if (status != Status::Ok) {
    Serial.printf("Failed to set factory settings: %s\r\n", statusToString(status));
    while (true)
      ;
  }

  status = lora.setAirDataRate400_900(AirDataRate400_900::Kbps62_5, false);
  if (status != Status::Ok) {
    Serial.printf("Failed to set air data rate: %s\r\n", statusToString(status));
    while (true)
      ;
  }

  status = lora.setEncryptionKey(ENCRYPTION_KEY, false);
  if (status != Status::Ok) {
    Serial.printf("Failed to set encryption key: %s\r\n", statusToString(status));
    while (true)
      ;
  }

  status = lora.setTransmissionMode(TxMode::Fixed, false);
  if (status != Status::Ok) {
    Serial.printf("Failed to set fixed mode: %s\r\n", statusToString(status));
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
  Serial.printf("Encryption key: 0x%04X (write-only)\r\n", ENCRYPTION_KEY);
  Serial.printf("Destination -> address: 0x%04X, channel: 0x%02X\r\n\n",
    DEST_ADDRESS,
    DEST_CHANNEL);
  Serial.println("Available commands:");
  Serial.println("> 's': Send an encrypted message\r\n");
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
      Serial.println("Sending encrypted message...");

      static uint32_t send_count = 0;
      char msg[32];
      snprintf(msg, sizeof(msg), "Encrypted msg #%lu", ++send_count);

      Status s = lora.sendFixedData(DEST_ADDRESS,
        DEST_CHANNEL,
        reinterpret_cast<const uint8_t*>(msg),
        strlen(msg));

      if (s != Status::Ok) {
        Serial.printf("[msg #%lu] Send failed: %s\r\n", send_count, statusToString(s));
      } else {
        Serial.printf("[msg #%lu] Sent (encrypted): \"%s\"\r\n", send_count, msg);
      }
    } break;
  }
}
