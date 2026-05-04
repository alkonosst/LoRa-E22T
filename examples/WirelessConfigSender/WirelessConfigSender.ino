/**
 * SPDX-FileCopyrightText: 2026 Maximiliano Ramirez <maximiliano.ramirezbravo@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 */

/** Explanation of this example:
 * - This example demonstrates how to change a remote module's configuration over-the-air (OTA).
 * - The sender issues a wireless config command that overwrites all registers on every module
 *   listening on the same channel and network ID. No address filter is applied; it is a broadcast.
 * - After the OTA command, the sender also updates its own channel locally so both modules stay in
 *   sync.
 * - Pair this example with WirelessConfigReceiver running on a second module.
 * - Use the Serial Monitor to send the following commands:
 *   - `s`: Send a transparent test message on the current channel.
 *   - `u`: Send OTA config to change the remote channel from INITIAL_CHANNEL to NEW_CHANNEL.
 *   - 'q': Query the remote module's configuration over-the-air and print the results.
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

// Channels (900 MHz band: frequency [MHz] = 850.125 + channel)
static constexpr float BASE_FREQUENCY    = 850.125f;
static constexpr uint8_t INITIAL_CHANNEL = 0x12; // ch 18 = 868.125 MHz (factory default)
static constexpr uint8_t NEW_CHANNEL     = 0x13; // ch 19 = 869.125 MHz

// Instance
static LoRaE22T lora;
static uint8_t current_channel = INITIAL_CHANNEL;

void setup() {
  Serial.begin(115200);
  Serial1.begin(MODULE_BAUD, SERIAL_8N1, PIN_SERIAL_RX, PIN_SERIAL_TX);
  delay(2000);

  Serial.println("----------------------------------------");
  Serial.println("LoRa-E22T - WirelessConfigSender Example");
  Serial.println("----------------------------------------");

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
  Serial.println("Available commands:");
  Serial.println("> 's': Send a transparent message on the current channel");
  Serial.printf("> 'u': Send OTA config (remote channel: 0x%02X -> 0x%02X)\r\n",
    INITIAL_CHANNEL,
    NEW_CHANNEL);
  Serial.println("> 'q': Query remote config over-the-air\r\n");
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
      char msg[64];
      snprintf(msg,
        sizeof(msg),
        "Hello from sender #%lu (ch=0x%02X)",
        ++send_count,
        current_channel);

      Status s = lora.sendTransparentData(reinterpret_cast<const uint8_t*>(msg), strlen(msg));
      if (s != Status::Ok) {
        Serial.printf("[msg #%lu] Send failed: %s\r\n", send_count, statusToString(s));
      } else {
        Serial.printf("[msg #%lu] Sent: \"%s\"\r\n", send_count, msg);
      }
    } break;

    case 'u':
    {
      // Build the target config for the remote module.
      // All fields use LoRaE22TConfig struct defaults (which match factory defaults for 900 MHz)
      // except for the channel, which is changed to NEW_CHANNEL.
      LoRaE22TConfig remote_config;
      remote_config.channel = NEW_CHANNEL;

      Serial.printf("Sending OTA config (ch 0x%02X -> 0x%02X)...\r\n",
        current_channel,
        NEW_CHANNEL);

      Status status = lora.setMode(Mode::Configuration);
      if (status != Status::Ok) {
        Serial.printf("Failed to enter configuration mode: %s\r\n", statusToString(status));
        break;
      }

      status = lora.setWirelessConfig(remote_config, false);
      if (status != Status::Ok) {
        Serial.printf("OTA config failed: %s\r\n", statusToString(status));
        break;
      }
      Serial.println("OTA config sent.");

      // Update sender's own channel to match the remote
      status = lora.setChannel(NEW_CHANNEL, false);
      if (status != Status::Ok) {
        Serial.printf("Failed to update local channel: %s\r\n", statusToString(status));
        lora.setMode(Mode::Transmission);
        break;
      }

      status = lora.setMode(Mode::Transmission);
      if (status != Status::Ok) {
        Serial.printf("Failed to re-enter transmission mode: %s\r\n", statusToString(status));
        break;
      }

      current_channel = NEW_CHANNEL;
      Serial.printf("Both modules now on channel 0x%02X (%.3f MHz)\r\n",
        current_channel,
        BASE_FREQUENCY + current_channel);
    } break;

    case 'q':
    {
      Serial.println("Querying remote config over-the-air...");

      Status status = lora.setMode(Mode::Configuration);
      if (status != Status::Ok) {
        Serial.printf("Failed to enter configuration mode: %s\r\n", statusToString(status));
        break;
      }

      static LoRaE22TConfig config;
      status = lora.getWirelessConfig(config);
      if (status != Status::Ok) {
        Serial.printf("Failed to get wireless config: %s\r\n", statusToString(status));
        lora.setMode(Mode::Transmission);
        break;
      }

      Serial.println("--- Remote Module Configuration ---");
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
