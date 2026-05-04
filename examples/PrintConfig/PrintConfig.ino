/**
 * SPDX-FileCopyrightText: 2026 Maximiliano Ramirez <maximiliano.ramirezbravo@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 */

/** Explanation of this example:
 * - This example demonstrates how to initialize the LoRa module and print its current configuration
 *   to the Serial console.
 * - Use the Serial Monitor to send the following commands:
 *   - `p`: Print the current configuration of the module.
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
  Serial.println("LoRa-E22T - PrintConfig Example");
  Serial.println("-------------------------------");

  Status s = lora.begin(MODULE_MODEL, Serial1, PIN_E22_M0, PIN_E22_M1, PIN_E22_AUX);
  if (s != Status::Ok) {
    Serial.printf("Failed to initialize module: %s", statusToString(s));
    while (true)
      ;
  }

  s = lora.setMode(Mode::Configuration);
  if (s != Status::Ok) {
    Serial.printf("Failed to enter configuration mode: %s\r\n", statusToString(s));
    while (true)
      ;
  }

  Serial.println("Module initialized successfully!\r\n");
  Serial.println("Available commands:");
  Serial.println("> 'p': Print current configuration\r\n");
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
    case 'p':
    {
      Serial.println("Fetching configuration from module...");

      static LoRaE22TConfig config;
      Status s = lora.getConfig(config);
      if (s != Status::Ok) {
        Serial.printf("Failed to read config: %s\r\n", statusToString(s));
        break;
      }

      Serial.println("--- Module Configuration ---");
      Serial.printf("  Address:          0x%04X\r\n", config.address);
      Serial.printf("  Network ID:       0x%02X\r\n", config.network_id);
      Serial.printf("  Channel:          %u\r\n", config.channel);
      Serial.printf("  Air data rate:    %u (raw)\r\n", config.air_data_rate);
      Serial.printf("  TX mode:          %s\r\n",
        config.tx_mode == TxMode::Fixed ? "Fixed" : "Transparent");
      Serial.printf("  TX power:         %u (raw)\r\n", config.transmission_power);
      Serial.printf("  UART baud rate:   %u (raw)\r\n", static_cast<uint8_t>(config.baud_rate));
      Serial.printf("  Subpacket length: %u (raw)\r\n",
        static_cast<uint8_t>(config.subpacket_length));
      Serial.printf("  RSSI ambient:     %s\r\n", config.rssi_ambient ? "on" : "off");
      Serial.printf("  RSSI packet:      %s\r\n", config.rssi_packet ? "on" : "off");
      Serial.printf("  LBT:              %s\r\n", config.lbt_enabled ? "on" : "off");
      Serial.printf("  Relay:            %s\r\n", config.relay_enabled ? "on" : "off");
      Serial.printf("  WOR mode:         %s\r\n",
        config.wor_mode == WORMode::Transmitter ? "Transmitter" : "Receiver");
      Serial.printf("  WOR cycle:        %u (raw)\r\n",
        static_cast<uint8_t>(config.wor_cycle_time));
      Serial.printf("  WOR delay:        %u ms\r\n", config.wor_delay_ms);
      Serial.printf("  Encryption key:   0x%04X (write-only, always reads 0)\r\n",
        config.encryption_key);
    } break;
  }
}