/**
 * SPDX-FileCopyrightText: 2026 Maximiliano Ramirez <maximiliano.ramirezbravo@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 */

/** Explanation of this example:
 * - This example demonstrates receiving an over-the-air (OTA) configuration change from a remote
 *   module. When the sender issues a wireless config command, this module applies the new
 *   configuration automatically at the hardware level (no application-level handling needed).
 * - After the OTA config is applied the module will be on the new channel. The sender also updates
 *   its own channel, so communication continues without interruption.
 * - Pair this example with WirelessConfigSender running on a second module.
 * - Use the Serial Monitor to send the following commands:
 *   - `p`: Print the current configuration to verify the channel changed after the OTA command.
 */

#include <Arduino.h>

#include <LoRa-E22T.h>
using namespace E22;

// Pins
static constexpr uint8_t PIN_E22_M0    = 10;
static constexpr uint8_t PIN_E22_M1    = 1;
static constexpr uint8_t PIN_E22_AUX   = 2;
static constexpr int8_t PIN_E22_RESET  = -1; // not used in this example
static constexpr uint8_t PIN_SERIAL_TX = 0;
static constexpr uint8_t PIN_SERIAL_RX = 3;

// Module config
static constexpr Model MODULE_MODEL   = Model::E22_900T30;
static constexpr uint32_t MODULE_BAUD = 9600;

// Channels (900 MHz band: frequency [MHz] = 850.125 + channel)
static constexpr float BASE_FREQUENCY    = 850.125f;
static constexpr uint8_t INITIAL_CHANNEL = 0x12; // ch 18 = 868.125 MHz (factory default)

// Receive buffer
static constexpr size_t RX_BUFFER_SIZE = 256;
static uint8_t rx_buffer[RX_BUFFER_SIZE];

// Instance
static LoRaE22T lora;

void setup() {
  Serial.begin(115200);
  Serial1.begin(MODULE_BAUD, SERIAL_8N1, PIN_SERIAL_RX, PIN_SERIAL_TX);
  delay(2000);

  Serial.println("------------------------------------------");
  Serial.println("LoRa-E22T - WirelessConfigReceiver Example");
  Serial.println("------------------------------------------");

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

  status = lora.setMode(Mode::Transmission);
  if (status != Status::Ok) {
    Serial.printf("Failed to enter transmission mode: %s\r\n", statusToString(status));
    while (true)
      ;
  }

  Serial.printf("Module initialized on channel 0x%02X (%.3f MHz)\r\n\n",
    INITIAL_CHANNEL,
    BASE_FREQUENCY + INITIAL_CHANNEL);
  Serial.println("Listening for incoming messages and OTA config commands...");
  Serial.println("Available commands:");
  Serial.println("> 'p': Print current config (verify channel after OTA command)\r\n");
}

void loop() {
  // Print incoming transparent messages
  if (lora.isDataAvailable()) {
    size_t data_size = 0;
    Status s         = lora.readData(false, rx_buffer, RX_BUFFER_SIZE, data_size);
    if (s != Status::Ok) {
      Serial.printf("Read failed: %s\r\n", statusToString(s));
      return;
    }

    if (data_size == 0) return;

    static uint32_t receive_count = 0;
    Serial.printf("[msg #%lu] Received (%u bytes): \"%.*s\"\r\n",
      ++receive_count,
      data_size,
      static_cast<int>(data_size),
      reinterpret_cast<const char*>(rx_buffer));
  }

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
      Status status = lora.setMode(Mode::Configuration);
      if (status != Status::Ok) {
        Serial.printf("Failed to enter configuration mode: %s\r\n", statusToString(status));
        break;
      }

      static LoRaE22TConfig config;
      status = lora.getConfig(config);
      if (status != Status::Ok) {
        Serial.printf("Failed to read config: %s\r\n", statusToString(status));
        lora.setMode(Mode::Transmission);
        break;
      }

      Serial.println("--- Module Configuration ---");
      Serial.printf("  Address:        0x%04X\r\n", config.address);
      Serial.printf("  Network ID:     0x%02X\r\n", config.network_id);
      Serial.printf("  Channel:        0x%02X (%.3f MHz)\r\n",
        config.channel,
        BASE_FREQUENCY + config.channel);
      Serial.printf("  Air data rate:  %u (raw)\r\n", config.air_data_rate);
      Serial.printf("  TX mode:        %s\r\n",
        config.tx_mode == TxMode::Fixed ? "Fixed" : "Transparent");
      Serial.printf("  TX power:       %u (raw)\r\n", config.transmission_power);
      Serial.printf("  WOR mode:       %s\r\n",
        config.wor_mode == WORMode::Transmitter ? "Transmitter" : "Receiver");

      status = lora.setMode(Mode::Transmission);
      if (status != Status::Ok) {
        Serial.printf("Failed to return to transmission mode: %s\r\n", statusToString(status));
        break;
      }
    } break;
  }
}
