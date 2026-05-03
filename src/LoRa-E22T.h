#pragma once

#include "LoRa-E22T-Types.h"
#include <HardwareSerial.h>

namespace E22 {

class LoRaE22T {
  public:
  LoRaE22T() = default;

  /* -------------------------------- Initialization and control -------------------------------- */

  /**
   * @brief Initialize the LoRa-E22T module with the given model, serial port, and control pins.
   * Must be called before any other methods. The module will be configured to start in Transmission
   * mode.
   *
   * @param model Specific LoRa-E22T model being used.
   * @param serial HardwareSerial port connected to the module's UART pins. Must be initialized by
   * the caller.
   * @param m0 GPIO pin connected to the module's M0 pin.
   * @param m1 GPIO pin connected to the module's M1 pin.
   * @param aux GPIO pin connected to the module's AUX pin.
   * @param pin_reset GPIO pin connected to the module's RESET pin. Optional, set to -1 if not used.
   * @return `Status::Ok` if success, error status otherwise.
   */
  Status begin(const Model model, HardwareSerial& serial, const int8_t m0, const int8_t m1,
    const int8_t aux, const int8_t pin_reset = -1);

  /**
   * @brief Pulse the RESET pin LOW then wait for the module to restart. Requires `pin_reset != -1`.
   * @return `Status::Ok` if success, error status otherwise.
   */
  Status reset();

  /**
   * @brief Set the operating mode of the module.
   * @param mode Target `Mode` to switch to.
   * @return `Status::Ok` if success, error status otherwise.
   */
  Status setMode(const Mode mode);

  /**
   * @brief Get the current operating mode of the module.
   * @param mode Output parameter for the current `Mode`.
   * @return `Status::Ok` if success, error status otherwise.
   */
  Status getMode(Mode& mode) const;

  /* ----------------------------------- Configuration setters ---------------------------------- */

  /**
   * @brief Restore module to factory default parameters.
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
   *
   * @return `Status::Ok` if success, error status otherwise.
   *
   * @note Requires module to be in `Mode::Configuration` mode.
   */
  Status setFactorySettings();

  /**
   * @brief Write all registers (0x00-0x0A) from config in a single transaction.
   * @param config `LoRaE22TConfig` instance containing the configuration values to write.
   * @param persistent Save to flash if true, temporary until reset if false.
   * @return `Status::Ok` if success, error status otherwise.
   *
   * @note Requires module to be in `Mode::Configuration` mode.
   */
  Status setConfig(const LoRaE22TConfig& config, const bool persistent = false);

  /**
   * @brief Send a wireless configuration command to a remote module over-the-air.
   * @param config `LoRaE22TConfig` instance containing the configuration values to send.
   * @param persistent Save to flash on the remote if true, temporary until reset if false.
   * @return `Status::Ok` if success, error status otherwise.
   *
   * @note Requires module to be in `Mode::Configuration` mode.
   */
  Status setWirelessConfig(const LoRaE22TConfig& config, const bool persistent = false);

  /**
   * @brief Set the 16-bit address of the module.
   * @param address 16-bit module address.
   * @param persistent Save to flash if true, temporary until reset if false.
   * @return `Status::Ok` if success, error status otherwise.
   *
   * @note Requires module to be in `Mode::Configuration` mode.
   */
  Status setAddress(const uint16_t address, const bool persistent = false);

  /**
   * @brief Set the network ID of the module. Modules on the same network must share this value.
   * @param network_id Network ID byte.
   * @param persistent Save to flash if true, temporary until reset if false.
   * @return `Status::Ok` if success, error status otherwise.
   *
   * @note Requires module to be in `Mode::Configuration` mode.
   */
  Status setNetworkID(uint8_t network_id, const bool persistent = false);

  /**
   * @brief Set the UART baud rate and parity of the module.
   * @param baud_rate UART baud rate.
   * @param parity UART parity mode.
   * @param persistent Save to flash if true, temporary until reset if false.
   * @return `Status::Ok` if success, error status otherwise.
   *
   * @note Requires module to be in `Mode::Configuration` mode.
   */
  Status setUARTConfig(const UARTBaudRate baud_rate, const UARTParity parity,
    const bool persistent = false);

  /**
   * @brief Set the air data rate for 230 MHz models.
   * @param rate Air data rate.
   * @param persistent Save to flash if true, temporary until reset if false.
   * @return `Status::Ok` if success, error status otherwise.
   *
   * @note Requires module to be in `Mode::Configuration` mode.
   */
  Status setAirDataRate230(const AirDataRate230 rate, const bool persistent = false);

  /**
   * @brief Set the air data rate for 400/900 MHz models.
   * @param rate Air data rate.
   * @param persistent Save to flash if true, temporary until reset if false.
   * @return `Status::Ok` if success, error status otherwise.
   *
   * @note Requires module to be in `Mode::Configuration` mode.
   */
  Status setAirDataRate400_900(const AirDataRate400_900 rate, const bool persistent = false);

  /**
   * @brief Set the maximum subpacket size for fragmented transmission.
   * @param subpacket_length Maximum subpacket size.
   * @param persistent Save to flash if true, temporary until reset if false.
   * @return `Status::Ok` if success, error status otherwise.
   *
   * @note Requires module to be in `Mode::Configuration` mode.
   */
  Status setSubpacketLength(const SubpacketLength subpacket_length, const bool persistent = false);

  /**
   * @brief Enable or disable ambient RSSI noise reporting.
   * @param enable true to enable, false to disable.
   * @param persistent Save to flash if true, temporary until reset if false.
   * @return `Status::Ok` if success, error status otherwise.
   *
   * @note Requires module to be in `Mode::Configuration` mode.
   */
  Status setAmbientRSSI(const bool enable, const bool persistent = false);

  /**
   * @brief Set the transmission power for 22 dBm models.
   * @param power Transmission power level.
   * @param persistent Save to flash if true, temporary until reset if false.
   * @return `Status::Ok` if success, error status otherwise.
   *
   * @note Requires module to be in `Mode::Configuration` mode.
   */
  Status setTransmissionPower22dBm(const TxPower22dBm power, const bool persistent = false);

  /**
   * @brief Set the transmission power for 30 dBm models.
   * @param power Transmission power level.
   * @param persistent Save to flash if true, temporary until reset if false.
   * @return `Status::Ok` if success, error status otherwise.
   *
   * @note Requires module to be in `Mode::Configuration` mode.
   */
  Status setTransmissionPower30dBm(const TxPower30dBm power, const bool persistent = false);

  /**
   * @brief Set the RF channel number.
   *
   * The valid channel numbers depend on the module frequency band:
   * - 230MHz models: 0-64 (frequency [MHz] = 220.125 + channel * 0.25)
   * - 400MHz models: 0-83 (frequency [MHz] = 410.125 + channel)
   * - 900MHz models: 0-80 (frequency [MHz] = 850.125 + channel)
   *
   * @param channel RF channel number.
   * @param persistent Save to flash if true, temporary until reset if false.
   * @return `Status::Ok` if success, error status otherwise.
   *
   * @note Requires module to be in `Mode::Configuration` mode.
   */
  Status setChannel(uint8_t channel, const bool persistent = false);

  /**
   * @brief Enable or disable appending the RSSI byte to each received packet.
   * @param enable true to enable, false to disable.
   * @param persistent Save to flash if true, temporary until reset if false.
   * @return `Status::Ok` if success, error status otherwise.
   *
   * @note Requires module to be in `Mode::Configuration` mode.
   */
  Status setPacketRSSI(const bool enable, const bool persistent = false);

  /**
   * @brief Set the transmission mode (Transparent or Fixed).
   * @param tx_mode Transmission mode.
   * @param persistent Save to flash if true, temporary until reset if false.
   * @return `Status::Ok` if success, error status otherwise.
   *
   * @note Requires module to be in `Mode::Configuration` mode.
   */
  Status setTransmissionMode(const TxMode tx_mode, const bool persistent = false);

  /**
   * @brief Enable or disable relay mode.
   * @param enable true to enable, false to disable.
   * @param persistent Save to flash if true, temporary until reset if false.
   * @return `Status::Ok` if success, error status otherwise.
   *
   * @note Requires module to be in `Mode::Configuration` mode.
   */
  Status setRelayMode(const bool enable, const bool persistent = false);

  /**
   * @brief Enable or disable listen-before-talk (LBT).
   * @param enable true to enable, false to disable.
   * @param persistent Save to flash if true, temporary until reset if false.
   * @return `Status::Ok` if success, error status otherwise.
   *
   * @note Requires module to be in `Mode::Configuration` mode.
   */
  Status setListenBeforeTalk(const bool enable, const bool persistent = false);

  /**
   * @brief Set the Wake-on-Radio (WOR) mode.
   * @param wor_mode WOR mode (Receiver or Transmitter).
   * @param persistent Save to flash if true, temporary until reset if false.
   * @return `Status::Ok` if success, error status otherwise.
   *
   * @note Requires module to be in `Mode::Configuration` mode.
   */
  Status setWORMode(const WORMode wor_mode, const bool persistent = false);

  /**
   * @brief Set the WOR wake cycle period.
   * @param wor_cycle_time WOR cycle time.
   * @param persistent Save to flash if true, temporary until reset if false.
   * @return `Status::Ok` if success, error status otherwise.
   *
   * @note Requires module to be in `Mode::Configuration` mode.
   */
  Status setWORCycleTime(const WORCycleTime wor_cycle_time, const bool persistent = false);

  /**
   * @brief Set the 16-bit wireless encryption key.
   * @param key 16-bit encryption key. Set to 0 to disable encryption.
   * @param persistent Save to flash if true, temporary until reset if false.
   * @return `Status::Ok` if success, error status otherwise.
   *
   * @note Requires module to be in `Mode::Configuration` mode.
   */
  Status setEncryptionKey(const uint16_t key, const bool persistent = false);

  /**
   * @brief Set the WOR response window delay in milliseconds.
   * @param delay_ms Delay in milliseconds (0-65535).
   * @param persistent Save to flash if true, temporary until reset if false.
   * @return `Status::Ok` if success, error status otherwise.
   *
   * @note Requires module to be in `Mode::Configuration` mode.
   */
  Status setWORDelay(const uint16_t delay_ms, const bool persistent = false);

  /* ----------------------------------- Configuration getters ---------------------------------- */

  /**
   * @brief Read all registers (0x00-0x0A) into config in a single transaction.
   * @param config `LoRaE22TConfig` instance to hold the read configuration values.
   * @return `Status::Ok` if success, error status otherwise.
   *
   * @note Requires module to be in `Mode::Configuration` mode.
   */
  Status getConfig(LoRaE22TConfig& config);

  /**
   * @brief Get the 16-bit address of the module.
   * @param address Output parameter for the module address.
   * @return `Status::Ok` if success, error status otherwise.
   *
   * @note Requires module to be in `Mode::Configuration` mode.
   */
  Status getAddress(uint16_t& address);

  /**
   * @brief Get the network ID of the module.
   * @param network_id Output parameter for the network ID byte.
   * @return `Status::Ok` if success, error status otherwise.
   *
   * @note Requires module to be in `Mode::Configuration` mode.
   */
  Status getNetworkID(uint8_t& network_id);

  /**
   * @brief Get the UART baud rate and parity of the module.
   * @param baud_rate Output parameter for the UART baud rate.
   * @param parity Output parameter for the UART parity mode.
   * @return `Status::Ok` if success, error status otherwise.
   *
   * @note Requires module to be in `Mode::Configuration` mode.
   */
  Status getUARTConfig(UARTBaudRate& baud_rate, UARTParity& parity);

  /**
   * @brief Get the air data rate as a raw register value.
   * @param raw_rate Output parameter for the raw air data rate register value.
   * @return `Status::Ok` if success, error status otherwise.
   *
   * @note Requires module to be in `Mode::Configuration` mode.
   */
  Status getAirDataRate(uint8_t& raw_rate);

  /**
   * @brief Get the maximum subpacket size.
   * @param subpacket_length Output parameter for the subpacket length.
   * @return `Status::Ok` if success, error status otherwise.
   *
   * @note Requires module to be in `Mode::Configuration` mode.
   */
  Status getSubpacketLength(SubpacketLength& subpacket_length);

  /**
   * @brief Get whether ambient RSSI noise reporting is enabled.
   * @param enabled Output parameter, true if ambient RSSI is enabled.
   * @return `Status::Ok` if success, error status otherwise.
   *
   * @note Requires module to be in `Mode::Configuration` mode.
   */
  Status getAmbientRSSI(bool& enabled);

  /**
   * @brief Get the transmission power as a raw register value.
   * @param raw_power Output parameter for the raw transmission power register value.
   * @return `Status::Ok` if success, error status otherwise.
   *
   * @note Requires module to be in `Mode::Configuration` mode.
   */
  Status getTransmissionPower(uint8_t& raw_power);

  /**
   * @brief Get the RF channel number.
   *
   * The returned channel number depend on the module frequency band:
   * - 230MHz models: 0-64 (frequency [MHz] = 220.125 + channel * 0.25)
   * - 400MHz models: 0-83 (frequency [MHz] = 410.125 + channel)
   * - 900MHz models: 0-80 (frequency [MHz] = 850.125 + channel)
   *
   * @param channel Output parameter for the RF channel number.
   * @return `Status::Ok` if success, error status otherwise.
   *
   * @note Requires module to be in `Mode::Configuration` mode.
   */
  Status getChannel(uint8_t& channel);

  /**
   * @brief Get whether RSSI byte appending on received packets is enabled.
   * @param enabled Output parameter, true if packet RSSI is enabled.
   * @return `Status::Ok` if success, error status otherwise.
   *
   * @note Requires module to be in `Mode::Configuration` mode.
   */
  Status getPacketRSSI(bool& enabled);

  /**
   * @brief Get the current transmission mode.
   * @param tx_mode Output parameter for the `TxMode`.
   * @return `Status::Ok` if success, error status otherwise.
   *
   * @note Requires module to be in `Mode::Configuration` mode.
   */
  Status getTransmissionMode(TxMode& tx_mode);

  /**
   * @brief Get whether relay mode is enabled.
   * @param enabled Output parameter, true if relay mode is enabled.
   * @return `Status::Ok` if success, error status otherwise.
   *
   * @note Requires module to be in `Mode::Configuration` mode.
   */
  Status getRelayMode(bool& enabled);

  /**
   * @brief Get whether listen-before-talk (LBT) is enabled.
   * @param enabled Output parameter, true if LBT is enabled.
   * @return `Status::Ok` if success, error status otherwise.
   *
   * @note Requires module to be in `Mode::Configuration` mode.
   */
  Status getListenBeforeTalk(bool& enabled);

  /**
   * @brief Get the current Wake-on-Radio (WOR) mode.
   * @param wor_mode Output parameter for the `WORMode`.
   * @return `Status::Ok` if success, error status otherwise.
   *
   * @note Requires module to be in `Mode::Configuration` mode.
   */
  Status getWORMode(WORMode& wor_mode);

  /**
   * @brief Get the WOR wake cycle period.
   * @param wor_cycle_time Output parameter for the `WORCycleTime`.
   * @return `Status::Ok` if success, error status otherwise.
   *
   * @note Requires module to be in `Mode::Configuration` mode.
   */
  Status getWORCycleTime(WORCycleTime& wor_cycle_time);

  /**
   * @brief Get the WOR response window delay in milliseconds.
   * @param delay_ms Output parameter for the delay in milliseconds.
   * @return `Status::Ok` if success, error status otherwise.
   *
   * @note Requires module to be in `Mode::Configuration` mode.
   */
  Status getWORDelay(uint16_t& delay_ms);

  /* ------------------------------------- Data transmission ------------------------------------ */

  /**
   * @brief Send data in transparent mode.
   * @param data Pointer to the data buffer to send.
   * @param data_size Number of bytes to send.
   * @return `Status::Ok` if success, error status otherwise.
   *
   * @note Requires module to be in `TxMode::Transparent` mode.
   */
  Status sendTransparentData(const uint8_t* data, const size_t data_size);

  /**
   * @brief Send data to a specific address and channel in fixed mode.
   * @param address Destination 16-bit module address.
   * @param channel Destination RF channel.
   * @param data Pointer to the data buffer to send.
   * @param data_size Number of bytes to send.
   * @return `Status::Ok` if success, error status otherwise.
   *
   * @note Requires module to be in `TxMode::Fixed` mode.
   */
  Status sendFixedData(const uint16_t address, const uint8_t channel, const uint8_t* data,
    const size_t data_size);

  /**
   * @brief Broadcast data to all modules on a given channel in fixed mode.
   * @param channel Destination RF channel.
   * @param data Pointer to the data buffer to send.
   * @param data_size Number of bytes to send.
   * @return `Status::Ok` if success, error status otherwise.
   *
   * @note Requires module to be in `TxMode::Fixed` mode.
   */
  Status sendBroadcastFixedData(const uint8_t channel, const uint8_t* data, const size_t data_size);

  /**
   * @brief Check if data is available to read from the module.
   * @return true if data is available, false otherwise.
   */
  bool isDataAvailable() const;

  /**
   * @brief Read received data from the module's serial buffer.
   * @param includes_rssi If true, the last byte is an RSSI value that will be stripped and stored
   * internally. Requires `rssi_packet` to be enabled.
   * @param buffer Output buffer to store the received data.
   * @param buffer_size Size of the output buffer in bytes.
   * @param data_size Output parameter for the number of bytes actually read.
   * @return `Status::Ok` if success, `Status::BufferTooSmall` if the buffer is too small, error
   * status otherwise.
   */
  Status readData(const bool includes_rssi, uint8_t* buffer, const size_t buffer_size,
    size_t& data_size);

  /**
   * @brief Get the RSSI of the last received packet in dBm.
   * @return `int16_t` RSSI value in dBm.
   *
   * @note Requires `rssi_packet` to be enabled.
   */
  int16_t getLastPacketRSSI() const;

  /**
   * @brief Read the current ambient noise RSSI in dBm.
   * @param rssi_dbm Output parameter for the ambient RSSI in dBm.
   * @return `Status::Ok` if success, error status otherwise.
   *
   * @note Requires module to be in Transmission mode. Requires `rssi_ambient` to be enabled.
   */
  Status readAmbientRSSI(int16_t& rssi_dbm);

  private:
  bool _initialized       = false;
  Model _model            = Model::None;
  HardwareSerial* _serial = nullptr;
  int8_t _m0              = -1;
  int8_t _m1              = -1;
  int8_t _aux             = -1;
  int8_t _pin_reset       = -1;
  Mode _current_mode      = Mode::Transmission;
  uint8_t _last_rssi      = 0;

  enum class Command : uint8_t {
    SetRegister          = 0xC0,
    ReadRegister         = 0xC1,
    SetTemporaryRegister = 0xC2,
  };

  // Wait for AUX pin to go HIGH, indicating the module is ready for the next command
  Status _waitAuxHigh(const bool pre_delay = true, const uint8_t post_delay_ms = 0);

  // Check if the module is in the required mode
  Status _checkMode(const Mode required_mode);

  // Send a command to the module with the given parameters and read the response
  Status _sendCmd(const Command cmd, const Register reg_start, const uint8_t length, uint8_t* data);

  // Read a sequence of registers starting from reg_start into the provided output buffer
  Status _readRegisters(const Register reg_start, const uint8_t length, uint8_t* out);

  // Write a sequence of registers starting from reg_start with the provided data buffer
  Status _writeRegisters(const Register reg_start, const uint8_t length, uint8_t* data,
    const bool persistent);
};

} // namespace E22