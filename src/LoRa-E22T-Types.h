#pragma once

#include <cstdint>

namespace E22 {

enum class Model : uint8_t {
  None,        // No model specified
  E22_230T22S, // 230MHz, 22dBm
  E22_230T30S, // 230MHz, 30dBm
  E22_400T22S, // 400MHz, 22dBm
  E22_400T30S, // 400MHz, 30dBm
  E22_900T22S, // 900MHz, 22dBm
  E22_900T30S, // 900MHz, 30dBm
};

enum class Mode : uint8_t {
  Transmission,
  WakeOnRadio,
  Configuration,
  DeepSleep,
};

enum class Register : uint8_t {
  Addr_H  = 0x00,
  Addr_L  = 0x01,
  NetID   = 0x02,
  Reg0    = 0x03,
  Reg1    = 0x04,
  Reg2    = 0x05,
  Reg3    = 0x06,
  Crypt_H = 0x07,
  Crypt_L = 0x08,
  WD_H    = 0x09,
  WD_L    = 0x0A,
  PID     = 0x80,
};

enum class UARTBaudRate : uint8_t {
  B1200_000,
  B2400_001,
  B4800_010,
  B9600_011_Default,
  B19200_100,
  B38400_101,
  B57600_110,
  B115200_111
};

enum class UARTParity : uint8_t {
  P8N1_00_Default,
  P8O1_01,
  P8E1_10,
  P8N1_11,
};

enum class SubpacketLength : uint8_t {
  SL240_00_Default,
  SL128_01,
  SL64_10,
  SL32_11,
};

enum class WORCycleTime : uint8_t {
  W500MS_000,
  W1000MS_001,
  W1500MS_010,
  W2000MS_011_Default,
  W2500MS_100,
  W3000MS_101,
  W3500MS_110,
  W4000MS_111
};

enum class TxMode : uint8_t {
  Transparent,
  Fixed,
};

enum class WORMode : uint8_t {
  Receiver,
  Transmitter,
};

// Status codes X-macro
#define E22_STATUS_LIST \
  X(Ok)                 \
  X(ModelNotSet)        \
  X(SerialNotSet)       \
  X(PinsNotSet)         \
  X(Uninitialized)      \
  X(AuxTimeout)

#define X(name) name,
enum class Status : uint8_t { E22_STATUS_LIST };
#undef X

const char* statusToString(const Status status);

} // namespace E22