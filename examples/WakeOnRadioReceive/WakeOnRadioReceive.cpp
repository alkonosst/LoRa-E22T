/**
 * SPDX-FileCopyrightText: 2026 Maximiliano Ramirez <maximiliano.ramirezbravo@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 */

/** Explanation of this example:
 * - This example demonstrates how to configure the module as a WOR (Wake-on-Radio) receiver.
 * - In WOR receiver mode, the module sleeps and periodically wakes up to listen for a preamble sent
 *   by a WOR transmitter. This significantly reduces average power consumption.
 * - The WOR cycle time must be the same on both sender and receiver. A longer cycle reduces average
 *   power but increases latency.
 * - Pair this example with WakeOnRadioSend running on a second module.
 * - No interactive commands are needed; the module receives automatically whenever it wakes and
 *   detects the preamble from the sender.
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

// WOR cycle: must match the sender. Longer cycle = lower power but higher latency.
// Using the longest cycle for this example.
static constexpr WORCycleTime WOR_CYCLE = WORCycleTime::Ms4000;

// Channel: must match the sender
static constexpr uint8_t MY_CHANNEL = 0x12; // channel 18 = 868.125 MHz for 900 MHz models

// Receive buffer
static constexpr size_t RX_BUFFER_SIZE = 256;
static uint8_t rx_buffer[RX_BUFFER_SIZE];

// Instance
static LoRaE22T lora;

void setup() {
  Serial.begin(115200);
  Serial1.begin(MODULE_BAUD, SERIAL_8N1, PIN_SERIAL_RX, PIN_SERIAL_TX);
  delay(2000);

  Serial.println("--------------------------------------");
  Serial.println("LoRa-E22T - WakeOnRadioReceive Example");
  Serial.println("--------------------------------------");

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

  // Configure as WOR receiver with matching cycle time
  status = lora.setWORMode(WORMode::Receiver, false);
  if (status != Status::Ok) {
    Serial.printf("Failed to set WOR receiver mode: %s\r\n", statusToString(status));
    while (true)
      ;
  }

  status = lora.setWORCycleTime(WOR_CYCLE, false);
  if (status != Status::Ok) {
    Serial.printf("Failed to set WOR cycle time: %s\r\n", statusToString(status));
    while (true)
      ;
  }

  status = lora.setChannel(MY_CHANNEL, false);
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

  // WOR receiver enters low-power mode via Mode::WakeOnRadio
  status = lora.setMode(Mode::WakeOnRadio);
  if (status != Status::Ok) {
    Serial.printf("Failed to enter WOR mode: %s\r\n", statusToString(status));
    while (true)
      ;
  }

  Serial.println("Module initialized successfully!\r\n");
  Serial.printf("WOR cycle: %u (raw), channel: 0x%02X\r\n",
    static_cast<uint8_t>(WOR_CYCLE),
    MY_CHANNEL);
  Serial.println("Listening in WOR mode. Waiting for preamble from sender...\r\n");
}

void loop() {
  if (!lora.isDataAvailable()) return;

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
