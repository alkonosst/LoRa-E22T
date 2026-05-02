#pragma once

#include "LoRa-E22T-Types.h"
#include <HardwareSerial.h>

namespace E22 {

class LoRaE22TConfig {
  public:
  LoRaE22TConfig() = default;
};

class LoRaE22T {
  public:
  LoRaE22T() = default;

  /* -------------------------------------- Initialization -------------------------------------- */

  Status begin(const Model model, HardwareSerial& serial, const int8_t m0, const int8_t m1,
    const int8_t aux, const uint8_t aux_pin_delay_ms = 5, const uint16_t aux_timeout_ms = 500);

  /* --------------------------------------- Configuration -------------------------------------- */

  Status setFactorySettings();

  Status setMode(const Mode mode);
  Status setAddress(const uint16_t address);
  Status setNetworkID(const uint8_t network_id);
  Status setUARTConfig(const UARTBaudRate baud_rate, const UARTParity parity);
  Status setAirDataRate(const uint8_t air_data_rate);
  Status setSubpacketLength(const SubpacketLength subpacket_length);
  Status setEnvironmentalRSSI(const bool enable);
  Status setPacketRSSI(const bool enable);
  Status setTransmissionPower(const uint8_t power);
  Status setChannel(const uint8_t channel);
  Status setTransmissionMode(const TxMode tx_mode);
  Status setRepeaterMode(const bool enable);
  Status setListenBeforeTalk(const bool enable);
  Status setWORMode(const WORMode wor_mode);
  Status setWORCycleTime(const WORCycleTime wor_cycle_time);
  Status setEncryptionKey(const uint16_t key);

  Status getMode(Mode& mode);
  Status getAddress(uint16_t& address);
  Status getNetworkID(uint8_t& network_id);
  Status getUARTConfig(UARTBaudRate& baud_rate, UARTParity& parity);
  Status getAirDataRate(uint8_t& air_data_rate);
  Status getSubpacketLength(SubpacketLength& subpacket_length);
  Status getEnvironmentalRSSI(bool& enabled);
  Status getPacketRSSI(bool& enabled);
  Status getTransmissionPower(uint8_t& power);
  Status getChannel(uint8_t& channel);
  Status getTransmissionMode(TxMode& tx_mode);
  Status getRepeaterMode(bool& enabled);
  Status getListenBeforeTalk(bool& enabled);
  Status getWORMode(WORMode& wor_mode);
  Status getWORCycleTime(WORCycleTime& wor_cycle_time);
  Status getEncryptionKey(uint16_t& key);

  /* ------------------------------------- Data transmission ------------------------------------ */

  Status sendTransparentData(const uint8_t* data, const size_t data_size);
  Status sendFixedData(const uint8_t channel, const uint16_t address, const uint8_t* data,
    const size_t data_size);
  Status sendBroadcastFixedData(const uint8_t channel, const uint8_t* data, const size_t data_size);

  bool isDataAvailable() const;
  Status readData(bool includes_rssi, uint8_t* buffer, const size_t buffer_size, size_t& data_size);

  int16_t getLastRSSI() const;

  private:
  bool _initialized         = false;
  Model _model              = Model::None;
  HardwareSerial* _serial   = nullptr;
  int8_t _m0                = -1;
  int8_t _m1                = -1;
  int8_t _aux               = -1;
  uint8_t _aux_pin_delay_ms = 0;
  uint16_t _aux_timeout_ms  = 0;
  uint8_t _last_rssi        = 0;

  enum class Command : uint8_t {
    SetRegister          = 0xC0,
    ReadRegister         = 0xC1,
    SetTemporaryRegister = 0xC2,
    WirelessConfig       = 0xCF,
    WrongFormat          = 0xFF
  };

  enum FrameIndex : uint8_t { IdxCommand, IdxStartAddress, IdxLength, IdxData };

  Status _checkMode(const Mode mode);
  Status _waitAuxHigh();
  Status _sendCmd(const Command cmd, const Register reg_start, const Register reg_end,
    uint8_t* data, const uint8_t data_size);
};

} // namespace E22