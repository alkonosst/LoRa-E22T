#include "LoRa-E22T.h"

namespace E22 {

/* --------------------------------------- Initialization --------------------------------------- */

Status LoRaE22T::begin(const Model model, HardwareSerial& serial, const int8_t m0, const int8_t m1,
  const int8_t aux, const uint8_t aux_pin_delay_ms, const uint16_t aux_timeout_ms) {
  if (_initialized) return Status::Ok;

  if (_model == Model::None) return Status::ModelNotSet;
  if (_serial == nullptr) return Status::SerialNotSet;
  if (_m0 == -1 || _m1 == -1 || _aux == -1) return Status::PinsNotSet;

  pinMode(_m0, OUTPUT);
  pinMode(_m1, OUTPUT);
  pinMode(_aux, INPUT);

  digitalWrite(_m0, LOW);
  digitalWrite(_m1, LOW);

  _aux_pin_delay_ms = aux_pin_delay_ms;
  _aux_timeout_ms   = aux_timeout_ms;

  Status status = _waitAuxHigh();
  if (status != Status::Ok) return status;

  _initialized = true;
  return status;
}

/* ---------------------------------------- Configuration --------------------------------------- */

Status LoRaE22T::setFactorySettings() { return Status::Ok; }

Status LoRaE22T::setMode(const Mode mode) {
  if (!_initialized) return Status::Uninitialized;

  switch (mode) {
    case Mode::Transmission:
      digitalWrite(_m0, LOW);
      digitalWrite(_m1, LOW);
      break;

    case Mode::WakeOnRadio:
      digitalWrite(_m0, HIGH);
      digitalWrite(_m1, LOW);
      break;

    case Mode::Configuration:
      digitalWrite(_m0, LOW);
      digitalWrite(_m1, HIGH);
      break;

    case Mode::DeepSleep:
      digitalWrite(_m0, HIGH);
      digitalWrite(_m1, HIGH);
      break;
  }

  return _waitAuxHigh();
}

Status LoRaE22T::setAddress(const uint16_t address) { return Status::Ok; }
Status LoRaE22T::setNetworkID(const uint8_t network_id) { return Status::Ok; }
Status LoRaE22T::setUARTConfig(const UARTBaudRate baud_rate, const UARTParity parity) {
  return Status::Ok;
}
Status LoRaE22T::setAirDataRate(const uint8_t air_data_rate) { return Status::Ok; }
Status LoRaE22T::setSubpacketLength(const SubpacketLength subpacket_length) { return Status::Ok; }
Status LoRaE22T::setEnvironmentalRSSI(const bool enable) { return Status::Ok; }
Status LoRaE22T::setPacketRSSI(const bool enable) { return Status::Ok; }
Status LoRaE22T::setTransmissionPower(const uint8_t power) { return Status::Ok; }
Status LoRaE22T::setChannel(const uint8_t channel) { return Status::Ok; }
Status LoRaE22T::setTransmissionMode(const TxMode tx_mode) { return Status::Ok; }
Status LoRaE22T::setRepeaterMode(const bool enable) { return Status::Ok; }
Status LoRaE22T::setListenBeforeTalk(const bool enable) { return Status::Ok; }
Status LoRaE22T::setWORMode(const WORMode wor_mode) { return Status::Ok; }
Status LoRaE22T::setWORCycleTime(const WORCycleTime wor_cycle_time) { return Status::Ok; }
Status LoRaE22T::setEncryptionKey(const uint16_t key) { return Status::Ok; }

Status LoRaE22T::getMode(Mode& mode) {
  if (!_initialized) return Status::Uninitialized;

  const bool m0_state = digitalRead(_m0);
  const bool m1_state = digitalRead(_m1);

  if (!m0_state && !m1_state)
    mode = Mode::Transmission;
  else if (m0_state && !m1_state)
    mode = Mode::WakeOnRadio;
  else if (!m0_state && m1_state)
    mode = Mode::Configuration;
  else if (m0_state && m1_state)
    mode = Mode::DeepSleep;

  return Status::Ok;
}

Status LoRaE22T::getAddress(uint16_t& address) { return Status::Ok; }
Status LoRaE22T::getNetworkID(uint8_t& network_id) { return Status::Ok; }
Status LoRaE22T::getUARTConfig(UARTBaudRate& baud_rate, UARTParity& parity) { return Status::Ok; }
Status LoRaE22T::getAirDataRate(uint8_t& air_data_rate) { return Status::Ok; }
Status LoRaE22T::getSubpacketLength(SubpacketLength& subpacket_length) { return Status::Ok; }
Status LoRaE22T::getEnvironmentalRSSI(bool& enabled) { return Status::Ok; }
Status LoRaE22T::getPacketRSSI(bool& enabled) { return Status::Ok; }
Status LoRaE22T::getTransmissionPower(uint8_t& power) { return Status::Ok; }
Status LoRaE22T::getChannel(uint8_t& channel) { return Status::Ok; }
Status LoRaE22T::getTransmissionMode(TxMode& tx_mode) { return Status::Ok; }
Status LoRaE22T::getRepeaterMode(bool& enabled) { return Status::Ok; }
Status LoRaE22T::getListenBeforeTalk(bool& enabled) { return Status::Ok; }
Status LoRaE22T::getWORMode(WORMode& wor_mode) { return Status::Ok; }
Status LoRaE22T::getWORCycleTime(WORCycleTime& wor_cycle_time) { return Status::Ok; }
Status LoRaE22T::getEncryptionKey(uint16_t& key) { return Status::Ok; }

/* -------------------------------------- Data transmission ------------------------------------- */

Status LoRaE22T::sendTransparentData(const uint8_t* data, const size_t data_size) {
  return Status::Ok;
}

Status LoRaE22T::sendFixedData(const uint8_t channel, const uint16_t address, const uint8_t* data,
  const size_t data_size) {
  return Status::Ok;
}

Status LoRaE22T::sendBroadcastFixedData(const uint8_t channel, const uint8_t* data,
  const size_t data_size) {
  return Status::Ok;
}

bool LoRaE22T::isDataAvailable() const { return false; }

Status LoRaE22T::readData(bool includes_rssi, uint8_t* buffer, const size_t buffer_size,
  size_t& data_size) {
  return Status::Ok;
}

int16_t LoRaE22T::getLastRSSI() const { return _last_rssi; }

/* --------------------------------------- Private methods -------------------------------------- */

Status LoRaE22T::_checkMode(const Mode mode) { return Status::Ok; }

Status LoRaE22T::_waitAuxHigh() {
  const uint32_t start_time = millis();

  // NOTE: delay before while() is necessary to avoid reading the same state
  delay(_aux_pin_delay_ms);

  while (digitalRead(_aux) == LOW) {
    if (millis() - start_time > _aux_timeout_ms) return Status::AuxTimeout;
  }

  return Status::Ok;
}

Status LoRaE22T::_sendCmd(const Command cmd, const Register reg_start, const Register reg_end,
  uint8_t* data, const uint8_t data_size) {
  return Status::Ok;
}

/* -------------------------------------------- Extra ------------------------------------------- */

const char* statusToString(const Status status) {
#define X(name) \
  case Status::name: return #name;
  switch (status) { E22_STATUS_LIST }
#undef X

  return "UnknownStatus";
}

} // namespace E22