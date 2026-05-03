#include <Arduino.h>
#include <unity.h>

#include <LoRa-E22T.h>

using namespace E22;

/* ---------------------------------------------------------------------------------------------- */
/*                                           Definitions                                          */
/* ---------------------------------------------------------------------------------------------- */

// Pins
static constexpr uint8_t PIN_E22_M0    = 10;
static constexpr uint8_t PIN_E22_M1    = 1;
static constexpr uint8_t PIN_E22_AUX   = 2;
static constexpr int8_t PIN_E22_RESET  = -1; // not used in this test
static constexpr uint8_t PIN_SERIAL_TX = 0;
static constexpr uint8_t PIN_SERIAL_RX = 3;

// Module config
static constexpr Model MODULE_MODEL   = Model::E22_900T30;
static constexpr uint32_t MODULE_BAUD = 9600;

// Factory default channel for 900MHz modules
static constexpr uint8_t FACTORY_CHANNEL_900 = 0x12;

// Expected payload - must match TEST_PAYLOAD in test_sender.cpp
static const uint8_t EXPECTED_PAYLOAD[]       = {0xAA, 0xBB, 0xCC, 0xDD};
static constexpr size_t EXPECTED_PAYLOAD_SIZE = sizeof(EXPECTED_PAYLOAD);

// How long to wait for data to arrive (must be greater than sender TX_SYNC_DELAY_MS + air time)
static constexpr uint32_t RX_TIMEOUT_MS = 10000;

// How long to confirm no data arrives (must be greater than sender TX_SYNC_DELAY_MS).
// Must match NO_RX_WAIT_MS in test_sender.cpp so both boards finish at the same time.
static constexpr uint32_t NO_RX_WAIT_MS = 5000;

// Module instance
static LoRaE22T lora;
static uint8_t rx_buf[64];

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

// Blocks until data is available or RX_TIMEOUT_MS elapses.
// Fails the test on timeout.
static void wait_for_rx_data() {
  const uint32_t start = millis();
  while (!lora.isDataAvailable()) {
    if (millis() - start > RX_TIMEOUT_MS) {
      TEST_FAIL_MESSAGE("Timeout: no data from sender");
      return;
    }
  }
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
  wait_for_rx_data();

  size_t received = 0;
  TEST_ASSERT_EQUAL(Status::Ok, lora.readData(false, rx_buf, sizeof(rx_buf), received));
  TEST_ASSERT_EQUAL(EXPECTED_PAYLOAD_SIZE, received);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(EXPECTED_PAYLOAD, rx_buf, EXPECTED_PAYLOAD_SIZE);
}

void test_transparent_data_with_packet_rssi() {
  enter_config_mode();
  TEST_ASSERT_EQUAL(Status::Ok, lora.setPacketRSSI(true));
  enter_transmission_mode();

  // Wait for data; clean up before failing so later tests are not affected.
  const uint32_t start = millis();
  while (!lora.isDataAvailable()) {
    if (millis() - start > RX_TIMEOUT_MS) {
      enter_config_mode();
      TEST_ASSERT_EQUAL(Status::Ok, lora.setPacketRSSI(false));
      enter_transmission_mode();
      TEST_FAIL_MESSAGE("Timeout: no data from sender");
      return;
    }
  }

  // readData strips the RSSI byte and stores it internally; data_size is payload only.
  size_t received = 0;
  TEST_ASSERT_EQUAL(Status::Ok, lora.readData(true, rx_buf, sizeof(rx_buf), received));
  TEST_ASSERT_EQUAL(EXPECTED_PAYLOAD_SIZE, received);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(EXPECTED_PAYLOAD, rx_buf, EXPECTED_PAYLOAD_SIZE);

  const int16_t rssi = lora.getLastPacketRSSI();
  TEST_ASSERT_GREATER_OR_EQUAL_INT16(-160, rssi);
  TEST_ASSERT_LESS_OR_EQUAL_INT16(0, rssi);

  enter_config_mode();
  TEST_ASSERT_EQUAL(Status::Ok, lora.setPacketRSSI(false));
  enter_transmission_mode();
}

void test_fixed_data() {
  wait_for_rx_data();

  size_t received = 0;
  TEST_ASSERT_EQUAL(Status::Ok, lora.readData(false, rx_buf, sizeof(rx_buf), received));
  TEST_ASSERT_EQUAL(EXPECTED_PAYLOAD_SIZE, received);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(EXPECTED_PAYLOAD, rx_buf, EXPECTED_PAYLOAD_SIZE);
}

void test_broadcast_fixed_data() {
  wait_for_rx_data();

  size_t received = 0;
  TEST_ASSERT_EQUAL(Status::Ok, lora.readData(false, rx_buf, sizeof(rx_buf), received));
  TEST_ASSERT_EQUAL(EXPECTED_PAYLOAD_SIZE, received);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(EXPECTED_PAYLOAD, rx_buf, EXPECTED_PAYLOAD_SIZE);
}

void test_transparent_data_different_channel_no_rx() {
  // Sender is transmitting on FACTORY_CHANNEL_900 + 1. Wait NO_RX_WAIT_MS and confirm
  // nothing arrives on this channel.
  const uint32_t start = millis();
  while (millis() - start < NO_RX_WAIT_MS) {
    if (lora.isDataAvailable()) {
      // Flush unexpected bytes before failing so buffer is clean for next run.
      size_t dummy = 0;
      lora.readData(false, rx_buf, sizeof(rx_buf), dummy);
      TEST_FAIL_MESSAGE("Received unexpected data from a different channel");
      return;
    }
  }
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
