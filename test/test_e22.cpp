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

// Delay parameters
static constexpr uint8_t AUX_PIN_DELAY_MS = 2;
static constexpr uint16_t AUX_TIMEOUT_MS  = 500;

// Module used for testing
static constexpr Model MODULE_MODEL = Model::E22_900T30;

// Config mode always communicates at 9600 8N1 per the E22 datasheet
static constexpr uint32_t MODULE_BAUD = 9600;

// Factory default channel for 900MHz models (register value 0x12)
static constexpr uint8_t FACTORY_CHANNEL_900 = 0x12;

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
/*                                         Initialization                                         */
/* ---------------------------------------------------------------------------------------------- */

void test_begin_succeeds() {
  Status s = lora.begin(MODULE_MODEL,
    Serial1,
    PIN_E22_M0,
    PIN_E22_M1,
    PIN_E22_AUX,
    PIN_E22_RESET,
    AUX_PIN_DELAY_MS,
    AUX_TIMEOUT_MS);
  TEST_ASSERT_EQUAL(Status::Ok, s);
}

void test_begin_already_initialized() {
  Status s = lora.begin(MODULE_MODEL,
    Serial1,
    PIN_E22_M0,
    PIN_E22_M1,
    PIN_E22_AUX,
    PIN_E22_RESET,
    AUX_PIN_DELAY_MS,
    AUX_TIMEOUT_MS);
  TEST_ASSERT_EQUAL(Status::AlreadyInitialized, s);
}

/* ---------------------------------------------------------------------------------------------- */
/*                                         statusToString                                         */
/* ---------------------------------------------------------------------------------------------- */

void test_status_to_string_all_values() {
  // Every Status code must return a non-null, non-empty string.
  // The X-macro expands to one assertion per status code automatically,
  // so new codes added to E22_STATUS_LIST are covered without changing this test.
#define X(name)                                       \
  TEST_ASSERT_NOT_NULL(statusToString(Status::name)); \
  TEST_ASSERT_GREATER_THAN(0, strlen(statusToString(Status::name)));
  E22_STATUS_LIST
#undef X
}

/* ---------------------------------------------------------------------------------------------- */
/*                                         Mode switching                                         */
/* ---------------------------------------------------------------------------------------------- */

void test_initial_mode_is_transmission() {
  Mode mode;
  Status s = lora.getMode(mode);
  TEST_ASSERT_EQUAL(Status::Ok, s);
  TEST_ASSERT_EQUAL(Mode::Transmission, mode);
}

void test_mode_switch_to_configuration() {
  Status s = lora.setMode(Mode::Configuration);
  TEST_ASSERT_EQUAL(Status::Ok, s);
  Mode mode;
  lora.getMode(mode);
  TEST_ASSERT_EQUAL(Mode::Configuration, mode);
  lora.setMode(Mode::Transmission);
}

void test_mode_switch_to_wor() {
  Status s = lora.setMode(Mode::WakeOnRadio);
  TEST_ASSERT_EQUAL(Status::Ok, s);
  Mode mode;
  lora.getMode(mode);
  TEST_ASSERT_EQUAL(Mode::WakeOnRadio, mode);
  lora.setMode(Mode::Transmission);
}

void test_mode_switch_to_deep_sleep() {
  Status s = lora.setMode(Mode::DeepSleep);
  TEST_ASSERT_EQUAL(Status::Ok, s);
  Mode mode;
  lora.getMode(mode);
  TEST_ASSERT_EQUAL(Mode::DeepSleep, mode);
  lora.setMode(Mode::Transmission);
}

/* ---------------------------------------------------------------------------------------------- */
/*                                           Mode guards                                          */
/* ---------------------------------------------------------------------------------------------- */

void test_config_calls_rejected_in_transmission_mode() {
  // Ensure Transmission mode
  lora.setMode(Mode::Transmission);

  uint16_t addr;
  TEST_ASSERT_EQUAL(Status::WrongMode, lora.getAddress(addr));
  uint8_t ch;
  TEST_ASSERT_EQUAL(Status::WrongMode, lora.getChannel(ch));
  TEST_ASSERT_EQUAL(Status::WrongMode, lora.setChannel(0x01));
}

void test_tx_calls_rejected_in_config_mode() {
  enter_config_mode();
  const uint8_t payload[] = {0x00};
  TEST_ASSERT_EQUAL(Status::WrongMode, lora.sendTransparentData(payload, sizeof(payload)));
  enter_transmission_mode();
}

/* ---------------------------------------------------------------------------------------------- */
/*                                        Factory settings                                        */
/* ---------------------------------------------------------------------------------------------- */

void test_factory_settings() {
  enter_config_mode();
  TEST_ASSERT_EQUAL(Status::Ok, lora.setFactorySettings());
  enter_transmission_mode();
}

/* ---------------------------------------------------------------------------------------------- */
/*                                   Individual setters/getters                                   */
/* ---------------------------------------------------------------------------------------------- */
// Notes:
// - All writes use persistent=false (default) so they survive only until power cycle. The module
//   register is still read back correctly in the same session. Each test restores the field to its
//   factory default before leaving.
// - Configuration mode always communicates at 9600 8N1, so changing the UART baud register and
//   restoring it before leaving config mode is safe.

void test_address_roundtrip() {
  enter_config_mode();
  TEST_ASSERT_EQUAL(Status::Ok, lora.setAddress(0x1234));
  uint16_t addr = 0;
  TEST_ASSERT_EQUAL(Status::Ok, lora.getAddress(addr));
  TEST_ASSERT_EQUAL_HEX16(0x1234, addr);
  lora.setAddress(0x0000); // restore factory default
  enter_transmission_mode();
}

void test_network_id_roundtrip() {
  enter_config_mode();
  TEST_ASSERT_EQUAL(Status::Ok, lora.setNetworkID(0xAB));
  uint8_t id = 0;
  TEST_ASSERT_EQUAL(Status::Ok, lora.getNetworkID(id));
  TEST_ASSERT_EQUAL_HEX8(0xAB, id);
  lora.setNetworkID(0x00); // restore factory default
  enter_transmission_mode();
}

void test_channel_roundtrip() {
  enter_config_mode();
  TEST_ASSERT_EQUAL(Status::Ok, lora.setChannel(0x10));
  uint8_t ch = 0;
  TEST_ASSERT_EQUAL(Status::Ok, lora.getChannel(ch));
  TEST_ASSERT_EQUAL_HEX8(0x10, ch);
  lora.setChannel(FACTORY_CHANNEL_900); // restore factory default
  enter_transmission_mode();
}

void test_uart_config_roundtrip() {
  enter_config_mode();
  TEST_ASSERT_EQUAL(Status::Ok, lora.setUARTConfig(UARTBaudRate::Baud115200, UARTParity::None));
  UARTBaudRate baud;
  UARTParity parity;
  TEST_ASSERT_EQUAL(Status::Ok, lora.getUARTConfig(baud, parity));
  TEST_ASSERT_EQUAL(UARTBaudRate::Baud115200, baud);
  TEST_ASSERT_EQUAL(UARTParity::None, parity);
  lora.setUARTConfig(UARTBaudRate::Baud9600, UARTParity::None); // restore factory default
  enter_transmission_mode();
}

void test_rssi_ambient_roundtrip() {
  enter_config_mode();
  TEST_ASSERT_EQUAL(Status::Ok, lora.setAmbientRSSI(true));
  bool enabled = false;
  TEST_ASSERT_EQUAL(Status::Ok, lora.getAmbientRSSI(enabled));
  TEST_ASSERT_TRUE(enabled);
  lora.setAmbientRSSI(false); // restore factory default
  enter_transmission_mode();
}

void test_transmission_mode_roundtrip() {
  enter_config_mode();
  TEST_ASSERT_EQUAL(Status::Ok, lora.setTransmissionMode(TxMode::Fixed));
  TxMode mode;
  TEST_ASSERT_EQUAL(Status::Ok, lora.getTransmissionMode(mode));
  TEST_ASSERT_EQUAL(TxMode::Fixed, mode);
  lora.setTransmissionMode(TxMode::Transparent); // restore factory default
  enter_transmission_mode();
}

void test_wor_cycle_time_roundtrip() {
  enter_config_mode();
  TEST_ASSERT_EQUAL(Status::Ok, lora.setWORCycleTime(WORCycleTime::Ms1000));
  WORCycleTime wct;
  TEST_ASSERT_EQUAL(Status::Ok, lora.getWORCycleTime(wct));
  TEST_ASSERT_EQUAL(WORCycleTime::Ms1000, wct);
  lora.setWORCycleTime(WORCycleTime::Ms2000); // restore factory default
  enter_transmission_mode();
}

void test_wor_delay_roundtrip() {
  enter_config_mode();
  TEST_ASSERT_EQUAL(Status::Ok, lora.setWORDelay(1500));
  uint16_t delay_ms = 0;
  TEST_ASSERT_EQUAL(Status::Ok, lora.getWORDelay(delay_ms));
  TEST_ASSERT_EQUAL_UINT16(1500, delay_ms);
  lora.setWORDelay(0); // restore factory default
  enter_transmission_mode();
}

/* ---------------------------------------------------------------------------------------------- */
/*                                  Full config struct roundtrip                                  */
/* ---------------------------------------------------------------------------------------------- */

void test_full_config_roundtrip() {
  enter_config_mode();

  LoRaE22TConfig cfg;
  cfg.address            = 0xBEEF;
  cfg.network_id         = 0x07;
  cfg.baud_rate          = UARTBaudRate::Baud9600; // keep at 9600 for safe restore
  cfg.parity             = UARTParity::None;
  cfg.air_data_rate      = static_cast<uint8_t>(AirDataRate400_900::Kbps9_6);
  cfg.subpacket_length   = SubpacketLength::Bytes128;
  cfg.rssi_ambient       = true;
  cfg.transmission_power = static_cast<uint8_t>(TxPower30dBm::dBm30);
  cfg.channel            = FACTORY_CHANNEL_900;
  cfg.rssi_packet        = false;
  cfg.tx_mode            = TxMode::Transparent;
  cfg.relay_enabled      = false;
  cfg.lbt_enabled        = false;
  cfg.wor_mode           = WORMode::Receiver;
  cfg.wor_cycle_time     = WORCycleTime::Ms2000;
  cfg.encryption_key     = 0x0000;
  cfg.wor_delay_ms       = 0;

  TEST_ASSERT_EQUAL(Status::Ok, lora.setConfig(cfg));

  LoRaE22TConfig rb;
  TEST_ASSERT_EQUAL(Status::Ok, lora.getConfig(rb));

  TEST_ASSERT_EQUAL_HEX16(cfg.address, rb.address);
  TEST_ASSERT_EQUAL_HEX8(cfg.network_id, rb.network_id);
  TEST_ASSERT_EQUAL(cfg.baud_rate, rb.baud_rate);
  TEST_ASSERT_EQUAL(cfg.parity, rb.parity);
  TEST_ASSERT_EQUAL_HEX8(cfg.air_data_rate, rb.air_data_rate);
  TEST_ASSERT_EQUAL(cfg.subpacket_length, rb.subpacket_length);
  TEST_ASSERT_EQUAL(cfg.rssi_ambient, rb.rssi_ambient);
  TEST_ASSERT_EQUAL_HEX8(cfg.transmission_power, rb.transmission_power);
  TEST_ASSERT_EQUAL_HEX8(cfg.channel, rb.channel);
  TEST_ASSERT_EQUAL(cfg.rssi_packet, rb.rssi_packet);
  TEST_ASSERT_EQUAL(cfg.tx_mode, rb.tx_mode);
  TEST_ASSERT_EQUAL(cfg.relay_enabled, rb.relay_enabled);
  TEST_ASSERT_EQUAL(cfg.lbt_enabled, rb.lbt_enabled);
  TEST_ASSERT_EQUAL(cfg.wor_mode, rb.wor_mode);
  TEST_ASSERT_EQUAL(cfg.wor_cycle_time, rb.wor_cycle_time);
  TEST_ASSERT_EQUAL_HEX16(0x0000, rb.encryption_key); // write-only
  TEST_ASSERT_EQUAL_UINT16(cfg.wor_delay_ms, rb.wor_delay_ms);

  lora.setFactorySettings(); // restore
  enter_transmission_mode();
}

/* ---------------------------------------------------------------------------------------------- */
/*                 Data transmission (single module - verifies API behavior only)                 */
/* ---------------------------------------------------------------------------------------------- */

void test_send_transparent_data_succeeds() {
  const uint8_t payload[] = {0xDE, 0xAD, 0xBE, 0xEF};
  TEST_ASSERT_EQUAL(Status::Ok, lora.sendTransparentData(payload, sizeof(payload)));
}

void test_is_data_available_returns_false_when_idle() {
  delay(200); // let any stale bytes settle
  while (Serial1.available())
    Serial1.read(); // flush
  TEST_ASSERT_FALSE(lora.isDataAvailable());
}

/* ---------------------------------------------------------------------------------------------- */
/*                                          setup / loop                                          */
/* ---------------------------------------------------------------------------------------------- */

void setup() {
  Serial.begin(115200);
  Serial1.begin(MODULE_BAUD, SERIAL_8N1, PIN_SERIAL_RX, PIN_SERIAL_TX);
  delay(1000);

  UNITY_BEGIN();

  // Initialization (must run first - all remaining tests depend on it)
  RUN_TEST(test_begin_succeeds);
  RUN_TEST(test_begin_already_initialized);

  // Status string coverage
  RUN_TEST(test_status_to_string_all_values);

  // Mode management
  RUN_TEST(test_initial_mode_is_transmission);
  RUN_TEST(test_mode_switch_to_configuration);
  RUN_TEST(test_mode_switch_to_wor);
  RUN_TEST(test_mode_switch_to_deep_sleep);

  // Mode guards
  RUN_TEST(test_config_calls_rejected_in_transmission_mode);
  RUN_TEST(test_tx_calls_rejected_in_config_mode);

  // Configuration
  RUN_TEST(test_factory_settings);
  RUN_TEST(test_address_roundtrip);
  RUN_TEST(test_network_id_roundtrip);
  RUN_TEST(test_channel_roundtrip);
  RUN_TEST(test_uart_config_roundtrip);
  RUN_TEST(test_rssi_ambient_roundtrip);
  RUN_TEST(test_transmission_mode_roundtrip);
  RUN_TEST(test_wor_cycle_time_roundtrip);
  RUN_TEST(test_wor_delay_roundtrip);
  RUN_TEST(test_full_config_roundtrip);

  // Data transmission
  RUN_TEST(test_send_transparent_data_succeeds);
  RUN_TEST(test_is_data_available_returns_false_when_idle);

  UNITY_END();
}

void loop() {}