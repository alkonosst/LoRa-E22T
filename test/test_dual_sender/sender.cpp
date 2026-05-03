#include <Arduino.h>
#include <unity.h>

#include <LoRa-E22T.h>

using namespace E22;

/* ---------------------------------------------------------------------------------------------- */
/*                                           Definitions                                          */
/* ---------------------------------------------------------------------------------------------- */

// Pins
static constexpr uint8_t PIN_E22_M0    = 9;
static constexpr uint8_t PIN_E22_M1    = 10;
static constexpr uint8_t PIN_E22_AUX   = 13;
static constexpr int8_t PIN_E22_RESET  = -1; // not used in this test
static constexpr uint8_t PIN_SERIAL_TX = 11;
static constexpr uint8_t PIN_SERIAL_RX = 12;

// Module config
static constexpr Model MODULE_MODEL   = Model::E22_900T30;
static constexpr uint32_t MODULE_BAUD = 9600;

// Factory default channel for 900MHz modules
static constexpr uint8_t FACTORY_CHANNEL_900 = 0x12;

// Receiver factory default address and channel
static constexpr uint16_t RECEIVER_ADDR   = 0x0000;
static constexpr uint8_t RECEIVER_CHANNEL = FACTORY_CHANNEL_900;

// Payload used in all TX tests
static const uint8_t TEST_PAYLOAD[]       = {0xAA, 0xBB, 0xCC, 0xDD};
static constexpr size_t TEST_PAYLOAD_SIZE = sizeof(TEST_PAYLOAD);

// Sender delays this long before each transmission so the receiver reaches its wait point first.
// Both boards are powered simultaneously; this gives the receiver enough head start.
static constexpr uint32_t TX_SYNC_DELAY_MS = 2000;

// Must match NO_RX_WAIT_MS in test_receiver.cpp.
// After transmitting on a wrong channel, sender idles for this long to stay in sync.
static constexpr uint32_t NO_RX_WAIT_MS = 5000;

// Module instance
static LoRaE22T lora;

/* ---------------------------------------------------------------------------------------------- */
/*                                             Helpers                                            */
/* ---------------------------------------------------------------------------------------------- */

static void enter_config_mode() {
  Status s = lora.setMode(Mode::Configuration);
  TEST_ASSERT_EQUAL_MESSAGE(Status::Ok, s, "Failed to enter Configuration mode");
}

static void enter_transmission_mode() {
  Status s = lora.setMode(Mode::Transmission);
  TEST_ASSERT_EQUAL_MESSAGE(Status::Ok, s, "Failed to enter Transmission mode");
}

/* ---------------------------------------------------------------------------------------------- */
/*                                             Tests                                              */
/* ---------------------------------------------------------------------------------------------- */

void test_begin_succeeds() {
  Status s = lora.begin(MODULE_MODEL, Serial1, PIN_E22_M0, PIN_E22_M1, PIN_E22_AUX, PIN_E22_RESET);
  TEST_ASSERT_EQUAL(Status::Ok, s);
}

void test_set_factory_settings() {
  enter_config_mode();
  TEST_ASSERT_EQUAL(Status::Ok, lora.setFactorySettings());
  enter_transmission_mode();
}

void test_transparent_data() {
  delay(TX_SYNC_DELAY_MS);
  TEST_ASSERT_EQUAL(Status::Ok, lora.sendTransparentData(TEST_PAYLOAD, TEST_PAYLOAD_SIZE));
}

void test_transparent_data_with_packet_rssi() {
  // Packet RSSI is configured on the receiver side; sender just transmits normally.
  delay(TX_SYNC_DELAY_MS);
  TEST_ASSERT_EQUAL(Status::Ok, lora.sendTransparentData(TEST_PAYLOAD, TEST_PAYLOAD_SIZE));
}

void test_fixed_data() {
  enter_config_mode();
  TEST_ASSERT_EQUAL(Status::Ok, lora.setTransmissionMode(TxMode::Fixed));
  enter_transmission_mode();

  delay(TX_SYNC_DELAY_MS);
  TEST_ASSERT_EQUAL(Status::Ok,
    lora.sendFixedData(RECEIVER_ADDR, RECEIVER_CHANNEL, TEST_PAYLOAD, TEST_PAYLOAD_SIZE));
}

void test_broadcast_fixed_data() {
  enter_config_mode();
  TEST_ASSERT_EQUAL(Status::Ok, lora.setTransmissionMode(TxMode::Fixed));
  enter_transmission_mode();

  delay(TX_SYNC_DELAY_MS);
  TEST_ASSERT_EQUAL(Status::Ok,
    lora.sendBroadcastFixedData(RECEIVER_CHANNEL, TEST_PAYLOAD, TEST_PAYLOAD_SIZE));
}

void test_transparent_data_different_channel_no_rx() {
  // Switch to a different channel so receiver (on FACTORY_CHANNEL_900) gets nothing.
  enter_config_mode();
  TEST_ASSERT_EQUAL(Status::Ok, lora.setTransmissionMode(TxMode::Transparent));
  TEST_ASSERT_EQUAL(Status::Ok, lora.setChannel(FACTORY_CHANNEL_900 + 1));
  enter_transmission_mode();

  delay(TX_SYNC_DELAY_MS);
  TEST_ASSERT_EQUAL(Status::Ok, lora.sendTransparentData(TEST_PAYLOAD, TEST_PAYLOAD_SIZE));
}

/* ---------------------------------------------------------------------------------------------- */
/*                                          setup / loop                                          */
/* ---------------------------------------------------------------------------------------------- */

void setup() {
  Serial.begin(115200);
  Serial1.begin(MODULE_BAUD, SERIAL_8N1, PIN_SERIAL_RX, PIN_SERIAL_TX);
  delay(2000);

  Serial.println("Starting tests...");

  UNITY_BEGIN();

  // Initialization
  RUN_TEST(test_begin_succeeds);
  RUN_TEST(test_set_factory_settings);

  // Comm tests
  RUN_TEST(test_transparent_data);
  RUN_TEST(test_transparent_data_with_packet_rssi);
  RUN_TEST(test_fixed_data);
  RUN_TEST(test_broadcast_fixed_data);
  RUN_TEST(test_transparent_data_different_channel_no_rx);

  UNITY_END();
}

void loop() {}
