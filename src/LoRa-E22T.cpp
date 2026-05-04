#include "LoRa-E22T.h"

namespace E22 {

// Reads a single register byte, modifies the bits specified by mask/shift, then writes it back.
static void _applyBits(uint8_t& reg_val, uint8_t value, uint8_t shift, uint8_t mask) {
  reg_val = (reg_val & ~(mask << shift)) | ((value & mask) << shift);
}

/* --------------------------------- Initialization and control --------------------------------- */

Status LoRaE22T::begin(const Model model, HardwareSerial& serial, const int8_t m0, const int8_t m1,
  const int8_t aux, const int8_t pin_reset) {
  if (_initialized) return Status::AlreadyInitialized;

  if (model == Model::None) return Status::ModelNotSet;
  if (m0 == -1 || m1 == -1 || aux == -1) return Status::PinsNotSet;

  _model     = model;
  _serial    = &serial;
  _m0        = m0;
  _m1        = m1;
  _aux       = aux;
  _pin_reset = pin_reset;

  pinMode(_m0, OUTPUT);
  pinMode(_m1, OUTPUT);
  pinMode(_aux, INPUT);

  // Start in Transmission mode (M0=0, M1=0)
  digitalWrite(_m0, LOW);
  digitalWrite(_m1, LOW);
  _current_mode = Mode::Transmission;

  if (_pin_reset != -1) {
    pinMode(_pin_reset, OUTPUT);
    digitalWrite(_pin_reset, HIGH);
  }

  Status status = _waitAuxHigh(true, _POST_MODE_CHANGE_DELAY_MS);
  if (status != Status::Ok) return status;

  _initialized = true;
  return Status::Ok;
}

Status LoRaE22T::reset() {
  if (!_initialized) return Status::Uninitialized;
  if (_pin_reset == -1) return Status::PinsNotSet;

  // Pull RESET LOW for at least 100us, then release
  digitalWrite(_pin_reset, LOW);
  delay(_RESET_PULSE_MS);
  digitalWrite(_pin_reset, HIGH);

  // Wait for module startup (T1 = 16ms typical, with margin)
  delay(_MODULE_STARTUP_MS);

  // Wait for AUX to go HIGH, signalling the module is ready
  return _waitAuxHigh(true, _POST_MODE_CHANGE_DELAY_MS);
}

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

  _current_mode = mode;

  return _waitAuxHigh(true, _POST_MODE_CHANGE_DELAY_MS);
}

Status LoRaE22T::getMode(Mode& mode) const {
  if (!_initialized) return Status::Uninitialized;
  mode = _current_mode;
  return Status::Ok;
}

/* ------------------------------------ Configuration setters ----------------------------------- */

Status LoRaE22T::setFactorySettings() {
  if (!_initialized) return Status::Uninitialized;

  Status status = _checkMode(Mode::Configuration);
  if (status != Status::Ok) return status;

  /**
   * Factory default register values per model (registers 0x00-0x08, 9 bytes):
   * Values:
   * - 230MHz: C0 00 09 00 00 00 63 00 28 03 00 00
   * - 400MHz: C0 00 09 00 00 00 62 00 17 03 00 00
   * - 900MHz: C0 00 09 00 00 00 62 00 12 03 00 00
   *
   * Global:
   * Address 0x0000, NetID 0x00, 9600 baud 8N1, 2.4kbps air data rate, 240-byte subpacket, RSSI off,
   * software mode off, max power, packet RSSI off, transparent mode, relay off, LBT off, WOR
   * receiver, 2000ms WOR cycle, no encryption, 0ms WOR delay.
   *
   * Per model:
   * - 230MHz: Channel 0x28 (ch 40 - 230.125MHz)
   * - 400MHz: Channel 0x17 (ch 23 - 433.125MHz)
   * - 900MHz: Channel 0x12 (ch 18 - 868.125MHz)
   */

  uint8_t reg0    = 0;
  uint8_t channel = 0;

  if (_model == Model::E22_230T22 || _model == Model::E22_230T30) {
    reg0    = 0x63;
    channel = 0x28;
  } else if (_model == Model::E22_400T22 || _model == Model::E22_400T30) {
    reg0    = 0x62;
    channel = 0x17;
  } else if (_model == Model::E22_900T22 || _model == Model::E22_900T30) {
    reg0    = 0x62;
    channel = 0x12;
  }

  uint8_t data[_REG_COUNT_MAIN] = {
    0x00,    // ADDH
    0x00,    // ADDL
    0x00,    // NETID
    reg0,    // REG0
    0x00,    // REG1
    channel, // REG2
    0x03,    // REG3
    0x00,    // CRYPT_H
    0x00     // CRYPT_L
  };

  return _writeRegisters(Register::AddrH, _REG_COUNT_MAIN, data, true);
}

Status LoRaE22T::setConfig(const LoRaE22TConfig& config, const bool persistent) {
  if (!_initialized) return Status::Uninitialized;

  Status status = _checkMode(Mode::Configuration);
  if (status != Status::Ok) return status;

  const uint8_t reg0 = (static_cast<uint8_t>(config.baud_rate) << 5) |
                       (static_cast<uint8_t>(config.parity) << 3) | (config.air_data_rate & 0x07);

  const uint8_t reg1 = (static_cast<uint8_t>(config.subpacket_length) << 6) |
                       (config.rssi_ambient ? (1 << 5) : 0) | (config.transmission_power & 0x03);

  const uint8_t reg3 = (config.rssi_packet ? (1 << 7) : 0) |
                       (static_cast<uint8_t>(config.tx_mode) << 6) |
                       (config.relay_enabled ? (1 << 5) : 0) | (config.lbt_enabled ? (1 << 4) : 0) |
                       (static_cast<uint8_t>(config.wor_mode) << 3) |
                       (static_cast<uint8_t>(config.wor_cycle_time) & 0x07);

  uint8_t data[_REG_COUNT_ALL] = {
    static_cast<uint8_t>(config.address >> 8),
    static_cast<uint8_t>(config.address & 0xFF),
    config.network_id,
    reg0,
    reg1,
    config.channel,
    reg3,
    static_cast<uint8_t>(config.encryption_key >> 8),
    static_cast<uint8_t>(config.encryption_key & 0xFF),
    static_cast<uint8_t>(config.wor_delay_ms >> 8),
    static_cast<uint8_t>(config.wor_delay_ms & 0xFF),
  };

  return _writeRegisters(Register::AddrH, _REG_COUNT_ALL, data, persistent);
}

Status LoRaE22T::setWirelessConfig(const LoRaE22TConfig& config, const bool persistent) {
  if (!_initialized) return Status::Uninitialized;
  Status status = _checkMode(Mode::Configuration);
  if (status != Status::Ok) return status;

  const uint8_t reg0 = (static_cast<uint8_t>(config.baud_rate) << 5) |
                       (static_cast<uint8_t>(config.parity) << 3) | (config.air_data_rate & 0x07);

  const uint8_t reg1 = (static_cast<uint8_t>(config.subpacket_length) << 6) |
                       (config.rssi_ambient ? (1 << 5) : 0) | (config.transmission_power & 0x03);

  const uint8_t reg3 = (config.rssi_packet ? (1 << 7) : 0) |
                       (static_cast<uint8_t>(config.tx_mode) << 6) |
                       (config.relay_enabled ? (1 << 5) : 0) | (config.lbt_enabled ? (1 << 4) : 0) |
                       (static_cast<uint8_t>(config.wor_mode) << 3) |
                       (static_cast<uint8_t>(config.wor_cycle_time) & 0x07);

  const uint8_t cmd_byte = persistent ? static_cast<uint8_t>(Command::SetRegister)
                                      : static_cast<uint8_t>(Command::SetTemporaryRegister);

  uint8_t data[_REG_COUNT_ALL] = {
    static_cast<uint8_t>(config.address >> 8),
    static_cast<uint8_t>(config.address & 0xFF),
    config.network_id,
    reg0,
    reg1,
    config.channel,
    reg3,
    static_cast<uint8_t>(config.encryption_key >> 8),
    static_cast<uint8_t>(config.encryption_key & 0xFF),
    static_cast<uint8_t>(config.wor_delay_ms >> 8),
    static_cast<uint8_t>(config.wor_delay_ms & 0xFF),
  };

  // Flush stale bytes before sending
  while (_serial->available())
    _serial->read();

  // Wireless config frame: CF CF <cmd> <start_addr> <length> <data...>
  _serial->write(static_cast<uint8_t>(0xCF));
  _serial->write(static_cast<uint8_t>(0xCF));
  _serial->write(cmd_byte);
  _serial->write(static_cast<uint8_t>(Register::AddrH));
  _serial->write(static_cast<uint8_t>(_REG_COUNT_ALL));
  _serial->write(data, _REG_COUNT_ALL);

  // Response: CF CF C1 <start_addr> <length> <data...>
  const uint8_t response_size = 2 + _FRAME_HEADER_SIZE + _REG_COUNT_ALL;
  const uint32_t start_time   = millis();

  size_t available = 0;
  while (available < response_size) {
    available = _serial->available();

    // Early exit if module responds with error response (0xFF 0xFF 0xFF)
    if (available >= 3 && static_cast<uint8_t>(_serial->peek()) == 0xFF) {
      _serial->read();
      _serial->read();
      _serial->read();
      return Status::CommandFailed;
    }

    if (millis() - start_time > _WIRELESS_RESPONSE_TIMEOUT_MS) return Status::AuxTimeout;
  }

  const uint8_t prefix1 = static_cast<uint8_t>(_serial->read());
  const uint8_t prefix2 = static_cast<uint8_t>(_serial->read());
  if (prefix1 != 0xCF || prefix2 != 0xCF) return Status::CommandFailed;

  // Drain remaining response bytes (header + data)
  for (uint8_t i = 0; i < _FRAME_HEADER_SIZE + _REG_COUNT_ALL; i++) {
    _serial->read();
  }

  return _waitAuxHigh();
}

Status LoRaE22T::setAddress(const uint16_t address, const bool persistent) {
  if (!_initialized) return Status::Uninitialized;
  Status status = _checkMode(Mode::Configuration);
  if (status != Status::Ok) return status;

  uint8_t data[2] = {static_cast<uint8_t>(address >> 8), static_cast<uint8_t>(address & 0xFF)};

  return _writeRegisters(Register::AddrH, 2, data, persistent);
}

Status LoRaE22T::setNetworkID(uint8_t network_id, const bool persistent) {
  if (!_initialized) return Status::Uninitialized;
  Status status = _checkMode(Mode::Configuration);
  if (status != Status::Ok) return status;

  return _writeRegisters(Register::NetID, 1, &network_id, persistent);
}

Status LoRaE22T::setUARTConfig(const UARTBaudRate baud_rate, const UARTParity parity,
  const bool persistent) {
  if (!_initialized) return Status::Uninitialized;
  Status status = _checkMode(Mode::Configuration);
  if (status != Status::Ok) return status;

  uint8_t reg0 = 0;
  status       = _sendCmd(Command::ReadRegister, Register::Reg0, 1, &reg0);
  if (status != Status::Ok) return status;

  _applyBits(reg0, static_cast<uint8_t>(baud_rate), 5, 0x07);
  _applyBits(reg0, static_cast<uint8_t>(parity), 3, 0x03);
  return _writeRegisters(Register::Reg0, 1, &reg0, persistent);
}

Status LoRaE22T::setAirDataRate230(const AirDataRate230 rate, const bool persistent) {
  if (!_initialized) return Status::Uninitialized;

  bool is_230MHz = (_model == Model::E22_230T22 || _model == Model::E22_230T30);
  if (!is_230MHz) return Status::WrongModel;

  Status status = _checkMode(Mode::Configuration);
  if (status != Status::Ok) return status;

  uint8_t reg0 = 0;
  status       = _readRegisters(Register::Reg0, 1, &reg0);
  if (status != Status::Ok) return status;

  _applyBits(reg0, static_cast<uint8_t>(rate), 0, 0x07);
  return _writeRegisters(Register::Reg0, 1, &reg0, persistent);
}

Status LoRaE22T::setAirDataRate400_900(const AirDataRate400_900 rate, const bool persistent) {
  if (!_initialized) return Status::Uninitialized;

  bool is_400_900MHz = (_model == Model::E22_400T22 || _model == Model::E22_400T30 ||
                        _model == Model::E22_900T22 || _model == Model::E22_900T30);
  if (!is_400_900MHz) return Status::WrongModel;

  Status status = _checkMode(Mode::Configuration);
  if (status != Status::Ok) return status;

  uint8_t reg0 = 0;
  status       = _readRegisters(Register::Reg0, 1, &reg0);
  if (status != Status::Ok) return status;

  _applyBits(reg0, static_cast<uint8_t>(rate), 0, 0x07);
  return _writeRegisters(Register::Reg0, 1, &reg0, persistent);
}

Status LoRaE22T::setSubpacketLength(const SubpacketLength subpacket_length, const bool persistent) {
  if (!_initialized) return Status::Uninitialized;
  Status status = _checkMode(Mode::Configuration);
  if (status != Status::Ok) return status;

  uint8_t reg1 = 0;
  status       = _readRegisters(Register::Reg1, 1, &reg1);
  if (status != Status::Ok) return status;

  _applyBits(reg1, static_cast<uint8_t>(subpacket_length), 6, 0x03);
  return _writeRegisters(Register::Reg1, 1, &reg1, persistent);
}

Status LoRaE22T::setAmbientRSSI(const bool enable, const bool persistent) {
  if (!_initialized) return Status::Uninitialized;
  Status status = _checkMode(Mode::Configuration);
  if (status != Status::Ok) return status;

  uint8_t reg1 = 0;
  status       = _readRegisters(Register::Reg1, 1, &reg1);
  if (status != Status::Ok) return status;

  _applyBits(reg1, enable ? 1 : 0, 5, 0x01);
  return _writeRegisters(Register::Reg1, 1, &reg1, persistent);
}

Status LoRaE22T::setTransmissionPower22dBm(const TxPower22dBm power, const bool persistent) {
  if (!_initialized) return Status::Uninitialized;

  bool is_22dBm =
    (_model == Model::E22_230T22 || _model == Model::E22_400T22 || _model == Model::E22_900T22);
  if (!is_22dBm) return Status::WrongModel;

  Status status = _checkMode(Mode::Configuration);
  if (status != Status::Ok) return status;

  uint8_t reg1 = 0;
  status       = _readRegisters(Register::Reg1, 1, &reg1);
  if (status != Status::Ok) return status;

  _applyBits(reg1, static_cast<uint8_t>(power), 0, 0x03);
  return _writeRegisters(Register::Reg1, 1, &reg1, persistent);
}

Status LoRaE22T::setTransmissionPower30dBm(const TxPower30dBm power, const bool persistent) {
  if (!_initialized) return Status::Uninitialized;

  bool is_30dBm =
    (_model == Model::E22_230T30 || _model == Model::E22_400T30 || _model == Model::E22_900T30);
  if (!is_30dBm) return Status::WrongModel;

  Status status = _checkMode(Mode::Configuration);
  if (status != Status::Ok) return status;

  uint8_t reg1 = 0;
  status       = _readRegisters(Register::Reg1, 1, &reg1);
  if (status != Status::Ok) return status;

  _applyBits(reg1, static_cast<uint8_t>(power), 0, 0x03);
  return _writeRegisters(Register::Reg1, 1, &reg1, persistent);
}

Status LoRaE22T::setChannel(uint8_t channel, const bool persistent) {
  if (!_initialized) return Status::Uninitialized;

  if (_model == Model::E22_230T22 || _model == Model::E22_230T30) {
    if (channel > 64) return Status::InvalidParameter; // 0-64 for 230MHz
  } else if (_model == Model::E22_400T22 || _model == Model::E22_400T30) {
    if (channel > 83) return Status::InvalidParameter; // 0-83 for 400MHz
  } else if (_model == Model::E22_900T22 || _model == Model::E22_900T30) {
    if (channel > 80) return Status::InvalidParameter; // 0-80 for 900MHz
  }

  Status status = _checkMode(Mode::Configuration);
  if (status != Status::Ok) return status;

  return _writeRegisters(Register::Reg2, 1, &channel, persistent);
}

Status LoRaE22T::setPacketRSSI(const bool enable, const bool persistent) {
  if (!_initialized) return Status::Uninitialized;
  Status status = _checkMode(Mode::Configuration);
  if (status != Status::Ok) return status;

  uint8_t reg3 = 0;
  status       = _readRegisters(Register::Reg3, 1, &reg3);
  if (status != Status::Ok) return status;

  _applyBits(reg3, enable ? 1 : 0, 7, 0x01);
  return _writeRegisters(Register::Reg3, 1, &reg3, persistent);
}

Status LoRaE22T::setTransmissionMode(const TxMode tx_mode, const bool persistent) {
  if (!_initialized) return Status::Uninitialized;
  Status status = _checkMode(Mode::Configuration);
  if (status != Status::Ok) return status;

  uint8_t reg3 = 0;
  status       = _readRegisters(Register::Reg3, 1, &reg3);
  if (status != Status::Ok) return status;

  _applyBits(reg3, static_cast<uint8_t>(tx_mode), 6, 0x01);
  return _writeRegisters(Register::Reg3, 1, &reg3, persistent);
}

Status LoRaE22T::setRelayMode(const bool enable, const bool persistent) {
  if (!_initialized) return Status::Uninitialized;
  Status status = _checkMode(Mode::Configuration);
  if (status != Status::Ok) return status;

  uint8_t reg3 = 0;
  status       = _readRegisters(Register::Reg3, 1, &reg3);
  if (status != Status::Ok) return status;

  _applyBits(reg3, enable ? 1 : 0, 5, 0x01);
  return _writeRegisters(Register::Reg3, 1, &reg3, persistent);
}

Status LoRaE22T::setListenBeforeTalk(const bool enable, const bool persistent) {
  if (!_initialized) return Status::Uninitialized;
  Status status = _checkMode(Mode::Configuration);
  if (status != Status::Ok) return status;

  uint8_t reg3 = 0;
  status       = _readRegisters(Register::Reg3, 1, &reg3);
  if (status != Status::Ok) return status;

  _applyBits(reg3, enable ? 1 : 0, 4, 0x01);
  return _writeRegisters(Register::Reg3, 1, &reg3, persistent);
}

Status LoRaE22T::setWORMode(const WORMode wor_mode, const bool persistent) {
  if (!_initialized) return Status::Uninitialized;
  Status status = _checkMode(Mode::Configuration);
  if (status != Status::Ok) return status;

  uint8_t reg3 = 0;
  status       = _readRegisters(Register::Reg3, 1, &reg3);
  if (status != Status::Ok) return status;

  _applyBits(reg3, static_cast<uint8_t>(wor_mode), 3, 0x01);
  return _writeRegisters(Register::Reg3, 1, &reg3, persistent);
}

Status LoRaE22T::setWORCycleTime(const WORCycleTime wor_cycle_time, const bool persistent) {
  if (!_initialized) return Status::Uninitialized;
  Status status = _checkMode(Mode::Configuration);
  if (status != Status::Ok) return status;

  uint8_t reg3 = 0;
  status       = _readRegisters(Register::Reg3, 1, &reg3);
  if (status != Status::Ok) return status;

  _applyBits(reg3, static_cast<uint8_t>(wor_cycle_time), 0, 0x07);
  return _writeRegisters(Register::Reg3, 1, &reg3, persistent);
}

Status LoRaE22T::setEncryptionKey(const uint16_t key, const bool persistent) {
  if (!_initialized) return Status::Uninitialized;
  Status status = _checkMode(Mode::Configuration);
  if (status != Status::Ok) return status;

  uint8_t data[2] = {static_cast<uint8_t>(key >> 8), static_cast<uint8_t>(key & 0xFF)};

  return _writeRegisters(Register::CryptH, 2, data, persistent);
}

Status LoRaE22T::setWORDelay(const uint16_t delay_ms, const bool persistent) {
  if (!_initialized) return Status::Uninitialized;
  Status status = _checkMode(Mode::Configuration);
  if (status != Status::Ok) return status;

  uint8_t data[2] = {static_cast<uint8_t>(delay_ms >> 8), static_cast<uint8_t>(delay_ms & 0xFF)};

  return _writeRegisters(Register::WdH, 2, data, persistent);
}

/* ------------------------------------ Configuration getters ----------------------------------- */

Status LoRaE22T::getConfig(LoRaE22TConfig& config) {
  if (!_initialized) return Status::Uninitialized;

  Status status = _checkMode(Mode::Configuration);
  if (status != Status::Ok) return status;

  uint8_t data[_REG_COUNT_ALL] = {};
  status                       = _readRegisters(Register::AddrH, _REG_COUNT_ALL, data);
  if (status != Status::Ok) return status;

  config.address    = (static_cast<uint16_t>(data[0]) << 8) | data[1];
  config.network_id = data[2];

  const uint8_t reg0   = data[3];
  config.baud_rate     = static_cast<UARTBaudRate>((reg0 >> 5) & 0x07);
  config.parity        = static_cast<UARTParity>((reg0 >> 3) & 0x03);
  config.air_data_rate = reg0 & 0x07;

  const uint8_t reg1        = data[4];
  config.subpacket_length   = static_cast<SubpacketLength>((reg1 >> 6) & 0x03);
  config.rssi_ambient       = (reg1 >> 5) & 0x01;
  config.transmission_power = reg1 & 0x03;

  config.channel = data[5];

  const uint8_t reg3    = data[6];
  config.rssi_packet    = (reg3 >> 7) & 0x01;
  config.tx_mode        = static_cast<TxMode>((reg3 >> 6) & 0x01);
  config.relay_enabled  = (reg3 >> 5) & 0x01;
  config.lbt_enabled    = (reg3 >> 4) & 0x01;
  config.wor_mode       = static_cast<WORMode>((reg3 >> 3) & 0x01);
  config.wor_cycle_time = static_cast<WORCycleTime>(reg3 & 0x07);

  // encryption_key is write-only; reads always return 0
  config.encryption_key = 0;

  config.wor_delay_ms = (static_cast<uint16_t>(data[9]) << 8) | data[10];

  return Status::Ok;
}

Status LoRaE22T::getAddress(uint16_t& address) {
  if (!_initialized) return Status::Uninitialized;
  Status status = _checkMode(Mode::Configuration);
  if (status != Status::Ok) return status;

  uint8_t data[2] = {};
  status          = _readRegisters(Register::AddrH, 2, data);
  if (status != Status::Ok) return status;

  address = (static_cast<uint16_t>(data[0]) << 8) | data[1];
  return Status::Ok;
}

Status LoRaE22T::getNetworkID(uint8_t& network_id) {
  if (!_initialized) return Status::Uninitialized;
  Status status = _checkMode(Mode::Configuration);
  if (status != Status::Ok) return status;

  return _readRegisters(Register::NetID, 1, &network_id);
}

Status LoRaE22T::getUARTConfig(UARTBaudRate& baud_rate, UARTParity& parity) {
  if (!_initialized) return Status::Uninitialized;
  Status status = _checkMode(Mode::Configuration);
  if (status != Status::Ok) return status;

  uint8_t reg0 = 0;
  status       = _readRegisters(Register::Reg0, 1, &reg0);
  if (status != Status::Ok) return status;

  baud_rate = static_cast<UARTBaudRate>((reg0 >> 5) & 0x07);
  parity    = static_cast<UARTParity>((reg0 >> 3) & 0x03);
  return Status::Ok;
}

Status LoRaE22T::getAirDataRate(uint8_t& raw_rate) {
  if (!_initialized) return Status::Uninitialized;
  Status status = _checkMode(Mode::Configuration);
  if (status != Status::Ok) return status;

  uint8_t reg0 = 0;
  status       = _readRegisters(Register::Reg0, 1, &reg0);
  if (status != Status::Ok) return status;

  raw_rate = reg0 & 0x07;
  return Status::Ok;
}

Status LoRaE22T::getSubpacketLength(SubpacketLength& subpacket_length) {
  if (!_initialized) return Status::Uninitialized;
  Status status = _checkMode(Mode::Configuration);
  if (status != Status::Ok) return status;

  uint8_t reg1 = 0;
  status       = _readRegisters(Register::Reg1, 1, &reg1);
  if (status != Status::Ok) return status;

  subpacket_length = static_cast<SubpacketLength>((reg1 >> 6) & 0x03);
  return Status::Ok;
}

Status LoRaE22T::getAmbientRSSI(bool& enabled) {
  if (!_initialized) return Status::Uninitialized;
  Status status = _checkMode(Mode::Configuration);
  if (status != Status::Ok) return status;

  uint8_t reg1 = 0;
  status       = _readRegisters(Register::Reg1, 1, &reg1);
  if (status != Status::Ok) return status;

  enabled = (reg1 >> 5) & 0x01;
  return Status::Ok;
}

Status LoRaE22T::getTransmissionPower(uint8_t& raw_power) {
  if (!_initialized) return Status::Uninitialized;
  Status status = _checkMode(Mode::Configuration);
  if (status != Status::Ok) return status;

  uint8_t reg1 = 0;
  status       = _readRegisters(Register::Reg1, 1, &reg1);
  if (status != Status::Ok) return status;

  raw_power = reg1 & 0x03;
  return Status::Ok;
}

Status LoRaE22T::getChannel(uint8_t& channel) {
  if (!_initialized) return Status::Uninitialized;
  Status status = _checkMode(Mode::Configuration);
  if (status != Status::Ok) return status;

  return _readRegisters(Register::Reg2, 1, &channel);
}

Status LoRaE22T::getPacketRSSI(bool& enabled) {
  if (!_initialized) return Status::Uninitialized;
  Status status = _checkMode(Mode::Configuration);
  if (status != Status::Ok) return status;

  uint8_t reg3 = 0;
  status       = _readRegisters(Register::Reg3, 1, &reg3);
  if (status != Status::Ok) return status;

  enabled = (reg3 >> 7) & 0x01;
  return Status::Ok;
}

Status LoRaE22T::getTransmissionMode(TxMode& tx_mode) {
  if (!_initialized) return Status::Uninitialized;
  Status status = _checkMode(Mode::Configuration);
  if (status != Status::Ok) return status;

  uint8_t reg3 = 0;
  status       = _readRegisters(Register::Reg3, 1, &reg3);
  if (status != Status::Ok) return status;

  tx_mode = static_cast<TxMode>((reg3 >> 6) & 0x01);
  return Status::Ok;
}

Status LoRaE22T::getRelayMode(bool& enabled) {
  if (!_initialized) return Status::Uninitialized;
  Status status = _checkMode(Mode::Configuration);
  if (status != Status::Ok) return status;

  uint8_t reg3 = 0;
  status       = _readRegisters(Register::Reg3, 1, &reg3);
  if (status != Status::Ok) return status;

  enabled = (reg3 >> 5) & 0x01;
  return Status::Ok;
}

Status LoRaE22T::getListenBeforeTalk(bool& enabled) {
  if (!_initialized) return Status::Uninitialized;
  Status status = _checkMode(Mode::Configuration);
  if (status != Status::Ok) return status;

  uint8_t reg3 = 0;
  status       = _readRegisters(Register::Reg3, 1, &reg3);
  if (status != Status::Ok) return status;

  enabled = (reg3 >> 4) & 0x01;
  return Status::Ok;
}

Status LoRaE22T::getWORMode(WORMode& wor_mode) {
  if (!_initialized) return Status::Uninitialized;
  Status status = _checkMode(Mode::Configuration);
  if (status != Status::Ok) return status;

  uint8_t reg3 = 0;
  status       = _readRegisters(Register::Reg3, 1, &reg3);
  if (status != Status::Ok) return status;

  wor_mode = static_cast<WORMode>((reg3 >> 3) & 0x01);
  return Status::Ok;
}

Status LoRaE22T::getWORCycleTime(WORCycleTime& wor_cycle_time) {
  if (!_initialized) return Status::Uninitialized;
  Status status = _checkMode(Mode::Configuration);
  if (status != Status::Ok) return status;

  uint8_t reg3 = 0;
  status       = _readRegisters(Register::Reg3, 1, &reg3);
  if (status != Status::Ok) return status;

  wor_cycle_time = static_cast<WORCycleTime>(reg3 & 0x07);
  return Status::Ok;
}

Status LoRaE22T::getWORDelay(uint16_t& delay_ms) {
  if (!_initialized) return Status::Uninitialized;
  Status status = _checkMode(Mode::Configuration);
  if (status != Status::Ok) return status;

  uint8_t data[2] = {};
  status          = _readRegisters(Register::WdH, 2, data);
  if (status != Status::Ok) return status;

  delay_ms = (static_cast<uint16_t>(data[0]) << 8) | data[1];
  return Status::Ok;
}

/* -------------------------------------- Data transmission ------------------------------------- */

Status LoRaE22T::sendTransparentData(const uint8_t* data, const size_t data_size) {
  if (!_initialized) return Status::Uninitialized;
  Status status = _checkMode(Mode::Transmission);
  if (status != Status::Ok) return status;

  _serial->write(data, data_size);
  return _waitAuxHigh();
}

Status LoRaE22T::sendFixedData(const uint16_t address, const uint8_t channel, const uint8_t* data,
  const size_t data_size) {
  if (!_initialized) return Status::Uninitialized;
  Status status = _checkMode(Mode::Transmission);
  if (status != Status::Ok) return status;

  // Frame: ADDH + ADDL + CH + data
  _serial->write(static_cast<uint8_t>(address >> 8));
  _serial->write(static_cast<uint8_t>(address & 0xFF));
  _serial->write(channel);
  _serial->write(data, data_size);
  return _waitAuxHigh();
}

Status LoRaE22T::sendBroadcastFixedData(const uint8_t channel, const uint8_t* data,
  const size_t data_size) {
  if (!_initialized) return Status::Uninitialized;
  Status status = _checkMode(Mode::Transmission);
  if (status != Status::Ok) return status;

  // Broadcast address is 0xFFFF
  _serial->write(static_cast<uint8_t>(0xFF));
  _serial->write(static_cast<uint8_t>(0xFF));
  _serial->write(channel);
  _serial->write(data, data_size);
  return _waitAuxHigh();
}

Status LoRaE22T::sendWORTransparentData(const uint8_t* data, const size_t data_size) {
  if (!_initialized) return Status::Uninitialized;
  Status status = _checkMode(Mode::WakeOnRadio);
  if (status != Status::Ok) return status;

  _serial->write(data, data_size);
  return _waitAuxHigh(true, 0, _AUX_WOR_TIMEOUT_MS);
}

Status LoRaE22T::sendWORFixedData(const uint16_t address, const uint8_t channel,
  const uint8_t* data, const size_t data_size) {
  if (!_initialized) return Status::Uninitialized;
  Status status = _checkMode(Mode::WakeOnRadio);
  if (status != Status::Ok) return status;

  // Frame: ADDH + ADDL + CH + data
  _serial->write(static_cast<uint8_t>(address >> 8));
  _serial->write(static_cast<uint8_t>(address & 0xFF));
  _serial->write(channel);
  _serial->write(data, data_size);
  return _waitAuxHigh(true, 0, _AUX_WOR_TIMEOUT_MS);
}

Status LoRaE22T::sendWORBroadcastFixedData(const uint8_t channel, const uint8_t* data,
  const size_t data_size) {
  if (!_initialized) return Status::Uninitialized;
  Status status = _checkMode(Mode::WakeOnRadio);
  if (status != Status::Ok) return status;

  // Broadcast address is 0xFFFF
  _serial->write(static_cast<uint8_t>(0xFF));
  _serial->write(static_cast<uint8_t>(0xFF));
  _serial->write(channel);
  _serial->write(data, data_size);
  return _waitAuxHigh(true, 0, _AUX_WOR_TIMEOUT_MS);
}

bool LoRaE22T::isDataAvailable() const {
  if (!_initialized) return false;
  return _serial->available() > 0;
}

Status LoRaE22T::readData(const bool includes_rssi, uint8_t* buffer, const size_t buffer_size,
  size_t& data_size) {
  if (!_initialized) return Status::Uninitialized;

  // Wait for AUX to go high indicating data is ready
  // pre_delay=false because if data is available, AUX will already be LOW and we can skip the
  // initial delay (AUX goes to LOW before data starts arriving to serial)
  Status status = _waitAuxHigh(false);
  if (status != Status::Ok) return status;

  const size_t available = _serial->available();
  if (available <= 0) {
    data_size = 0;
    return Status::Ok;
  }

  const size_t to_read = static_cast<size_t>(available);
  if (to_read > buffer_size) return Status::BufferTooSmall;

  for (size_t i = 0; i < to_read; i++) {
    buffer[i] = static_cast<uint8_t>(_serial->read());
  }

  if (includes_rssi && to_read > 0) {
    _last_rssi = buffer[to_read - 1];
    data_size  = to_read - 1;
  } else {
    data_size = to_read;
  }

  return Status::Ok;
}

int16_t LoRaE22T::getLastPacketRSSI() const { return -(256 - static_cast<int16_t>(_last_rssi)); }

Status LoRaE22T::readAmbientRSSI(int16_t& rssi_dbm) {
  if (!_initialized) return Status::Uninitialized;

  Status status1 = _checkMode(Mode::Transmission);
  Status status2 = _checkMode(Mode::WakeOnRadio);
  if (status1 != Status::Ok && status2 != Status::Ok) return Status::WrongMode;

  // Flush stale bytes before sending
  while (_serial->available())
    _serial->read();

  // Special command format: C0 C1 C2 C3 <start_addr> <length>
  // Register 0x00 = current ambient noise RSSI
  const uint8_t cmd_sequence[] = {0xC0, 0xC1, 0xC2, 0xC3, 0x00, 0x01};
  _serial->write(cmd_sequence, sizeof(cmd_sequence));

  // Response: C1 <addr> <len> <rssi_byte> = 4 bytes
  const uint32_t start_time = millis();

  size_t available = 0;
  while (available < 4) {
    available = _serial->available();

    // Early exit if module responds with error response (0xFF 0xFF 0xFF)
    if (available >= 3 && static_cast<uint8_t>(_serial->peek()) == 0xFF) {
      _serial->read();
      _serial->read();
      _serial->read();
      return Status::CommandFailed;
    }

    if (millis() - start_time > _SERIAL_RESPONSE_TIMEOUT_MS) return Status::AuxTimeout;
  }

  const uint8_t resp_cmd  = static_cast<uint8_t>(_serial->read());
  const uint8_t resp_addr = static_cast<uint8_t>(_serial->read());
  const uint8_t resp_len  = static_cast<uint8_t>(_serial->read());

  if (resp_cmd != static_cast<uint8_t>(Command::ReadRegister) || resp_addr != 0x00 ||
      resp_len != 0x01)
    return Status::CommandFailed;

  const uint8_t raw_rssi = static_cast<uint8_t>(_serial->read());
  rssi_dbm               = -(256 - static_cast<int16_t>(raw_rssi));

  return _waitAuxHigh();
}

/* --------------------------------------- Private methods -------------------------------------- */

Status LoRaE22T::_waitAuxHigh(const bool pre_delay, const uint8_t post_delay_ms,
  const uint16_t timeout_ms) {
  const uint32_t start_time = millis();

  if (pre_delay) delay(_AUX_PIN_DELAY_MS);

  while (digitalRead(_aux) == LOW) {
    if (millis() - start_time > timeout_ms) return Status::AuxTimeout;
  }

  if (post_delay_ms > 0) delay(post_delay_ms);

  // After the post-delay, check for a secondary AUX LOW pulse. This occurs when switching FROM
  // Configuration mode to another mode
  const uint32_t secondary_start = millis();
  while (digitalRead(_aux) == LOW) {
    if (millis() - secondary_start > timeout_ms) return Status::AuxTimeout;
  }

  return Status::Ok;
}

Status LoRaE22T::_checkMode(const Mode required_mode) {
  if (_current_mode != required_mode) return Status::WrongMode;
  return Status::Ok;
}

Status LoRaE22T::_sendCmd(const Command cmd, const Register reg_start, const uint8_t length,
  uint8_t* data) {
  // Flush any stale bytes before sending
  while (_serial->available())
    _serial->read();

  // Send frame: command + start address + length [+ data if write]
  _serial->write(static_cast<uint8_t>(cmd));
  _serial->write(static_cast<uint8_t>(reg_start));
  _serial->write(length);

  if (cmd == Command::SetRegister || cmd == Command::SetTemporaryRegister) {
    _serial->write(data, length);
  }

  // Wait for response: header (3 bytes) + data (length bytes)
  const size_t response_size = _FRAME_HEADER_SIZE + length;
  const uint32_t start_time  = millis();

  size_t available = 0;
  while (available < response_size) {
    available = _serial->available();

    // Early exit if module responds with error response (0xFF 0xFF 0xFF)
    if (available >= 3 && static_cast<uint8_t>(_serial->peek()) == 0xFF) {
      _serial->read();
      _serial->read();
      _serial->read();
      return Status::CommandFailed;
    }

    if (millis() - start_time > _SERIAL_RESPONSE_TIMEOUT_MS) return Status::SerialTimeout;
  }

  // Read and validate response header
  const uint8_t resp_cmd  = static_cast<uint8_t>(_serial->read());
  const uint8_t resp_addr = static_cast<uint8_t>(_serial->read());
  const uint8_t resp_len  = static_cast<uint8_t>(_serial->read());

  // For both read and write commands, the module responds with the same header format:
  // C1 <start_addr> <length>
  if (resp_cmd != static_cast<uint8_t>(Command::ReadRegister) ||
      resp_addr != static_cast<uint8_t>(reg_start) || resp_len != length) {
    return Status::CommandFailed;
  }

  // When reading, save response data into provided buffer.
  // When writing, just drain the echoed bytes.
  for (uint8_t i = 0; i < length; i++) {
    uint8_t b = static_cast<uint8_t>(_serial->read());
    if (cmd == Command::ReadRegister) {
      data[i] = b;
    }
  }

  return _waitAuxHigh();
}

Status LoRaE22T::_readRegisters(const Register reg_start, const uint8_t length, uint8_t* out) {
  return _sendCmd(Command::ReadRegister, reg_start, length, out);
}

Status LoRaE22T::_writeRegisters(const Register reg_start, const uint8_t length, uint8_t* data,
  const bool persistent) {
  Command cmd = persistent ? Command::SetRegister : Command::SetTemporaryRegister;
  return _sendCmd(cmd, reg_start, length, data);
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