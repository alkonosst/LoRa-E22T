/**
 * SPDX-FileCopyrightText: 2026 Maximiliano Ramirez <maximiliano.ramirezbravo@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 */

/** Explanation of this example:
 * - This example demonstrates how to send data to a WOR (Wake-on-Radio) receiver.
 * - In WOR mode, the receiver sleeps most of the time and wakes periodically to listen for a
 *   preamble. The sender must be configured as a WOR transmitter so it prepends the wake-up
 *   preamble automatically before transmitting data.
 * - The WOR cycle time must be the same on both sender and receiver. A longer cycle reduces average
 *   power on the receiver but increases latency.
 * - Pair this example with WakeOnRadioReceive running on a second module.
 * - Use the Serial Monitor to send the following commands:
 *   - `s`: Send a message to the WOR receiver (includes wake-up preamble).
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

// WOR cycle: must match the receiver. Longer cycle = lower power but higher latency.
// Using the longest cycle for this example.
static constexpr WORCycleTime WOR_CYCLE = WORCycleTime::Ms4000;

// Destination address and channel (transparent mode - address/channel must match the receiver)
static constexpr uint8_t DEST_CHANNEL = 0x12; // channel 18 = 868.125 MHz for 900 MHz models

// Instance
static LoRaE22T lora;

void setup() {
  Serial.begin(115200);
  Serial1.begin(MODULE_BAUD, SERIAL_8N1, PIN_SERIAL_RX, PIN_SERIAL_TX);
  delay(2000);

  Serial.println("-----------------------------------");
  Serial.println("LoRa-E22T - WakeOnRadioSend Example");
  Serial.println("-----------------------------------");

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

  // Configure as WOR transmitter and set matching cycle time
  status = lora.setWORMode(WORMode::Transmitter, false);
  if (status != Status::Ok) {
    Serial.printf("Failed to set WOR transmitter mode: %s\r\n", statusToString(status));
    while (true)
      ;
  }

  status = lora.setWORCycleTime(WOR_CYCLE, false);
  if (status != Status::Ok) {
    Serial.printf("Failed to set WOR cycle time: %s\r\n", statusToString(status));
    while (true)
      ;
  }

  status = lora.setChannel(DEST_CHANNEL, false);
  if (status != Status::Ok) {
    Serial.printf("Failed to set channel: %s\r\n", statusToString(status));
    while (true)
      ;
  }

  status = lora.setTransmissionMode(TxMode::Transparent, false);
  if (status != Status::Ok) {
    Serial.printf("Failed to set transparent mode: %s\r\n", statusToString(status));
    while (true)
      ;
  }

  // WOR transmitter operates via Mode::WakeOnRadio, not Mode::Transmission
  status = lora.setMode(Mode::WakeOnRadio);
  if (status != Status::Ok) {
    Serial.printf("Failed to enter WOR mode: %s\r\n", statusToString(status));
    while (true)
      ;
  }

  Serial.println("Module initialized successfully!\r\n");
  Serial.printf("WOR cycle: %u (raw), channel: 0x%02X\r\n\n",
    static_cast<uint8_t>(WOR_CYCLE),
    DEST_CHANNEL);
  Serial.println("Available commands:");
  Serial.println("> 's': Send a message to the WOR receiver\r\n");
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
      Serial.println("Sending WOR message...");

      static uint32_t send_count = 0;
      char msg[32];
      snprintf(msg, sizeof(msg), "WOR msg #%lu", ++send_count);

      Status s = lora.sendWORTransparentData(reinterpret_cast<const uint8_t*>(msg), strlen(msg));
      if (s != Status::Ok) {
        Serial.printf("[msg #%lu] Send failed: %s\r\n", send_count, statusToString(s));
      } else {
        Serial.printf("[msg #%lu] Sent: \"%s\"\r\n", send_count, msg);
      }
    } break;
  }
}
