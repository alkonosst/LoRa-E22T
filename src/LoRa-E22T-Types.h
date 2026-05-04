#pragma once

#include <cstdint>

namespace E22 {

/// @brief Models supported by this library.
enum class Model : uint8_t {
  None,       // No model specified
  E22_230T22, // 230MHz, 22dBm
  E22_230T30, // 230MHz, 30dBm
  E22_400T22, // 400MHz, 22dBm
  E22_400T30, // 400MHz, 30dBm
  E22_900T22, // 900MHz, 22dBm
  E22_900T30, // 900MHz, 30dBm
};

/// @brief Module operating modes.
enum class Mode : uint8_t {
  Transmission,  // Default mode for sending/receiving data.
  WakeOnRadio,   // Low-power mode where the module wakes on radio activity.
  Configuration, // Mode for reading/writing configuration registers.
  DeepSleep,     // Lowest power mode; module is unresponsive until mode is changed.
};

/// @brief Module configuration registers.
enum class Register : uint8_t {
  AddrH  = 0x00,
  AddrL  = 0x01,
  NetID  = 0x02,
  Reg0   = 0x03,
  Reg1   = 0x04,
  Reg2   = 0x05,
  Reg3   = 0x06,
  CryptH = 0x07,
  CryptL = 0x08,
  WdH    = 0x09,
  WdL    = 0x0A,
};

/// @brief Module UART baud rates.
enum class UARTBaudRate : uint8_t {
  Baud1200,
  Baud2400,
  Baud4800,
  Baud9600, // Default
  Baud19200,
  Baud38400,
  Baud57600,
  Baud115200,
};

/// @brief Module UART parity options.
enum class UARTParity : uint8_t {
  None, // 8N1 (default)
  Odd,  // 8O1
  Even, // 8E1
};

/**
 * @brief Maximum subpacket size for fragmented transmission. When the payload exceeds this size,
 * the module splits it into fragments of at most this many bytes and transmits them sequentially.
 * Smaller values free the RF channel more often between fragments, which can be useful in
 * multi-node networks or under duty-cycle regulations, but reduce throughput due to per-fragment RF
 * overhead. Leave at the default (240 bytes) unless you have a specific reason to reduce it.
 */
enum class SubpacketLength : uint8_t {
  Bytes240, // Default
  Bytes128,
  Bytes64,
  Bytes32,
};

/**
 * @brief Air data rates for 400MHz and 900MHz band models:
 * E22-400T22S, E22-400T30S, E22-900T22S, E22-900T30S
 */
enum class AirDataRate400_900 : uint8_t {
  Kbps2_4  = 2, // Default (values 0,1,2 all map to 2.4kbps; use index 2 as canonical)
  Kbps4_8  = 3,
  Kbps9_6  = 4,
  Kbps19_2 = 5,
  Kbps38_4 = 6,
  Kbps62_5 = 7,
};

/**
 * @brief Air data rates for 230MHz band models:
 * E22-230T22S, E22-230T30S
 */
enum class AirDataRate230 : uint8_t {
  Kbps2_4  = 3, // Default (values 0,1,2,3 all map to 2.4kbps; use index 3 as canonical)
  Kbps4_8  = 4,
  Kbps9_6  = 5,
  Kbps15_6 = 6, // Values 6 and 7 both map to 15.6kbps
};

/**
 * @brief Transmission modes for the module.
 *
 * In Transparent mode, data is sent/received without any additional processing or addressing.
 *
 * In Fixed mode, each packet includes a 2-byte address and 1-byte channel header, and received
 * packets are filtered by address and channel.
 */
enum class TxMode : uint8_t {
  Transparent,
  Fixed,
};

/**
 * @brief Transmission power levels for 22dBm models:
 * E22-230T22S, E22-400T22S, E22-900T22S
 */
enum class TxPower22dBm : uint8_t {
  dBm22 = 0, // Default
  dBm17 = 1,
  dBm14 = 2,
  dBm10 = 3,
};

/**
 * @brief Transmission power levels for 30dBm models:
 * E22-230T30S, E22-400T30S, E22-900T30S
 */
enum class TxPower30dBm : uint8_t {
  dBm30 = 0, // Default
  dBm27 = 1,
  dBm24 = 2,
  dBm21 = 3,
};

/**
 * @brief Wake-on-radio (WOR) modes. In WOR mode, the module periodically wakes up to listen for
 * radio activity.
 */
enum class WORMode : uint8_t {
  Receiver, // Default
  Transmitter,
};

/**
 * @brief Wake-on-radio (WOR) cycle times, which determine how long the module listens for radio
 * activity in WOR mode before going back to sleep. Longer cycle times reduce power consumption but
 * increase latency.
 */
enum class WORCycleTime : uint8_t {
  Ms500,
  Ms1000,
  Ms1500,
  Ms2000, // Default
  Ms2500,
  Ms3000,
  Ms3500,
  Ms4000,
};

/**
 * @brief Configuration struct for the LoRa-E22T module. This struct can be used to set or get all
 * module configuration parameters at once using setConfig() and getConfig().
 */
struct LoRaE22TConfig {
  uint16_t address                 = 0x0000;
  uint8_t network_id               = 0x00;
  UARTBaudRate baud_rate           = UARTBaudRate::Baud9600;
  UARTParity parity                = UARTParity::None;
  uint8_t air_data_rate            = 2; // Raw value; cast to AirDataRate400_900 or AirDataRate230
  SubpacketLength subpacket_length = SubpacketLength::Bytes240;
  bool rssi_ambient                = false;
  uint8_t transmission_power       = 0; // Raw value; cast to TxPower22dBm or TxPower30dBm
  uint8_t channel                  = 0x00;
  bool rssi_packet                 = false;
  TxMode tx_mode                   = TxMode::Transparent;
  bool relay_enabled               = false;
  bool lbt_enabled                 = false;
  WORMode wor_mode                 = WORMode::Receiver;
  WORCycleTime wor_cycle_time      = WORCycleTime::Ms2000;
  uint16_t encryption_key          = 0x0000; // Write-only; reads always return 0
  uint16_t wor_delay_ms            = 0;      // WOR receiver response window time (0-65535ms)
};

// Status codes X-macro
#define E22_STATUS_LIST \
  X(Ok)                 \
  X(ModelNotSet)        \
  X(SerialNotSet)       \
  X(PinsNotSet)         \
  X(Uninitialized)      \
  X(AlreadyInitialized) \
  X(AuxTimeout)         \
  X(SerialTimeout)      \
  X(WrongMode)          \
  X(WrongModel)         \
  X(InvalidParameter)   \
  X(CommandFailed)      \
  X(BufferTooSmall)

#define X(name) name,
enum class Status : uint8_t { E22_STATUS_LIST };
#undef X

/**
 * @brief Convert a `Status` code to a human-readable string for debugging and logging purposes.
 * @param status The `Status` code to convert.
 * @return `const char*` String representation of the status code.
 */
const char* statusToString(const Status status);

} // namespace E22