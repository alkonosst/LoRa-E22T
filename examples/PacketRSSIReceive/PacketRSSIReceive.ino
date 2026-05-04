/**
 * SPDX-FileCopyrightText: 2026 Maximiliano Ramirez <maximiliano.ramirezbravo@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 */

/** Explanation of this example:
 * - This example demonstrates how to receive data with the per-packet RSSI feature enabled.
 * - When packet RSSI is enabled, the module appends an extra RSSI byte at the end of each received
 *   packet. The library strips it automatically when readData() is called with includes_rssi=true.
 * - After receiving, the signal strength in dBm is printed alongside the message content.
 * - Pair this example with any sender (FixedSend, or BroadcastSend) running on a second module set
 *   to the same address/channel.
 * - No interactive commands are needed; the module receives automatically.
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

// This receiver's address and channel: must match DEST_ADDRESS/DEST_CHANNEL in FixedSend
static constexpr uint16_t MY_ADDRESS = 0x0001;
static constexpr uint8_t MY_CHANNEL  = 0x12; // channel 18 = 868.125 MHz for 900 MHz models

// Receive buffer
static constexpr size_t RX_BUFFER_SIZE = 256;
static uint8_t rx_buffer[RX_BUFFER_SIZE];

// Instance
static LoRaE22T lora;

void setup() {
  Serial.begin(115200);
  Serial1.begin(MODULE_BAUD, SERIAL_8N1, PIN_SERIAL_RX, PIN_SERIAL_TX);
  delay(2000);

  Serial.println("-------------------------------------");
  Serial.println("LoRa-E22T - PacketRSSIReceive Example");
  Serial.println("-------------------------------------");

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

  status = lora.setPacketRSSI(true, false);
  if (status != Status::Ok) {
    Serial.printf("Failed to enable packet RSSI: %s\r\n", statusToString(status));
    while (true)
      ;
  }

  status = lora.setAddress(MY_ADDRESS, false);
  if (status != Status::Ok) {
    Serial.printf("Failed to set address: %s\r\n", statusToString(status));
    while (true)
      ;
  }

  status = lora.setChannel(MY_CHANNEL, false);
  if (status != Status::Ok) {
    Serial.printf("Failed to set channel: %s\r\n", statusToString(status));
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
  Serial.println("Waiting for incoming messages with RSSI...\r\n");
}

void loop() {
  if (!lora.isDataAvailable()) return;

  size_t data_size = 0;

  // Pass includes_rssi=true so the library strips the trailing RSSI byte before returning data
  Status s = lora.readData(true, rx_buffer, RX_BUFFER_SIZE, data_size);
  if (s != Status::Ok) {
    Serial.printf("Read failed: %s\r\n", statusToString(s));
    return;
  }

  if (data_size == 0) return;

  static uint32_t receive_count = 0;
  int16_t rssi_dbm              = lora.getLastPacketRSSI();
  Serial.printf("[msg #%lu] Received (%u bytes): \"%.*s\" | RSSI: %d dBm\r\n",
    ++receive_count,
    data_size,
    static_cast<int>(data_size),
    reinterpret_cast<const char*>(rx_buffer),
    rssi_dbm);
}
