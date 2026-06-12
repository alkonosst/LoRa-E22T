<h1 align="center">
  <a><img src=".img/logo.svg" alt="Logo" width="300"></a>
  <br>
  LoRa-E22T
</h1>

<p align="center">
  <b>A clean and reliable Arduino library for the EByte E22-T series LoRa modules.</b>
</p>

<p align="center">
  <a href="https://www.ardu-badge.com/LoRa-E22T">
    <img src="https://www.ardu-badge.com/badge/LoRa-E22T.svg?" alt="Arduino Library Badge">
  </a>
  <a href="https://registry.platformio.org/libraries/alkonosst/LoRa-E22T">
    <img src="https://badges.registry.platformio.org/packages/alkonosst/library/LoRa-E22T.svg" alt="PlatformIO Registry">
  </a>
  <br><br>
  <a href="https://opensource.org/licenses/MIT">
    <img src="https://img.shields.io/badge/license-MIT-blue.svg?style=for-the-badge&color=blue" alt="License">
  </a>
  <br><br>
  <a href="https://ko-fi.com/alkonosst">
    <img src="https://ko-fi.com/img/githubbutton_sm.svg" alt="Ko-fi">
  </a>
</p>

---

# Table of contents <!-- omit in toc -->

- [Description](#description)
- [Key Features](#key-features)
- [Supported Modules](#supported-modules)
- [Quick Example](#quick-example)
- [Installation](#installation)
  - [PlatformIO](#platformio)
  - [Arduino IDE](#arduino-ide)
- [Usage](#usage)
  - [Including the library](#including-the-library)
  - [Namespace](#namespace)
  - [Initialization](#initialization)
  - [Operating Modes](#operating-modes)
  - [Configuration](#configuration)
    - [Setters](#setters)
    - [Getters](#getters)
    - [Bulk read/write](#bulk-readwrite)
    - [Persistent vs temporary](#persistent-vs-temporary)
    - [Channel frequencies](#channel-frequencies)
  - [Data Transmission](#data-transmission)
    - [Transparent mode](#transparent-mode)
    - [Fixed mode](#fixed-mode)
    - [Wake-on-Radio (WOR)](#wake-on-radio-wor)
  - [Data Reception](#data-reception)
    - [Reading data](#reading-data)
    - [Packet RSSI](#packet-rssi)
    - [Ambient RSSI](#ambient-rssi)
  - [Error Handling](#error-handling)
  - [Logging](#logging)
    - [Configuration macros](#configuration-macros)
- [Release Status](#release-status)
- [License](#license)

---

# Description

**LoRa-E22T** is an Arduino library for **EByte E22-T** series LoRa modules based on the **Semtech SX1262/SX1268** chipsets. It supports the full range of E22-T module variants across the **230 MHz**, **400 MHz**, and **900 MHz** frequency bands, at both **22 dBm** and **30 dBm** output power levels.

The library provides a complete API to configure the module, send and receive data in Transparent or Fixed addressing modes, use power-saving Wake-on-Radio (WOR), hardware encryption, and RSSI reporting - all with consistent, typed error handling.

# Key Features

- **Full model support** - All six E22-T variants across 230/400/900 MHz bands and 22/30 dBm power levels.
- **Complete configuration API** - Individual setters and getters for all registers, plus single-call bulk `setConfig()` / `getConfig()`.
- **Transparent and Fixed addressing** - Transparent mode for simple broadcast links; Fixed mode for addressed unicast and broadcast packets.
- **Wake-on-Radio (WOR)** - Dedicated send methods that handle the extended preamble timing automatically.
- **Hardware encryption** - 16-bit hardware encryption key (write-only register; no software overhead).
- **RSSI reporting** - Both per-packet RSSI (appended byte) and ambient noise floor measurement.
- **Informative status codes** - All methods return a typed `Status` enum. `statusToString()` converts it to a human-readable C string.
- **No dynamic memory** - Zero heap allocation. No `new`, `malloc`, or `String` anywhere in the library.

# Supported Modules

| Model       | Enum                | Band    | Max Power |
| ----------- | ------------------- | ------- | --------- |
| E22-230T22x | `Model::E22_230T22` | 230 MHz | 22 dBm    |
| E22-230T30x | `Model::E22_230T30` | 230 MHz | 30 dBm    |
| E22-400T22x | `Model::E22_400T22` | 400 MHz | 22 dBm    |
| E22-400T30x | `Model::E22_400T30` | 400 MHz | 30 dBm    |
| E22-900T22x | `Model::E22_900T22` | 900 MHz | 22 dBm    |
| E22-900T30x | `Model::E22_900T30` | 900 MHz | 30 dBm    |

# Quick Example

The following examples show a minimal point-to-point link in Transparent mode. Both modules must be on the same address and channel.

**Sender:**

```cpp
#include <Arduino.h>
#include <LoRa-E22T.h>
using namespace E22;

static LoRaE22T lora;

void setup() {
  Serial1.begin(9600, SERIAL_8N1, /*RX*/ 1, /*TX*/ 2);
  lora.begin(Model::E22_900T30, Serial1, /*M0*/ 3,  /*M1*/ 4, /*AUX*/ 5, /*RESET*/ -1);
}

void loop() {
  const char* msg = "Hello!";
  lora.sendTransparentData(reinterpret_cast<const uint8_t*>(msg), strlen(msg));
  delay(2000);
}
```

**Receiver:**

```cpp
#include <Arduino.h>
#include <LoRa-E22T.h>
using namespace E22;

static LoRaE22T lora;
static uint8_t buf[256];

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, /*RX*/ 1, /*TX*/ 2);
  lora.begin(Model::E22_900T30, Serial1, /*M0*/ 3,  /*M1*/ 4, /*AUX*/ 5, /*RESET*/ -1);
}

void loop() {
  if (!lora.isDataAvailable()) return;
  size_t received_bytes = 0;
  if (lora.readData(/*includes_rssi*/ false, buf, sizeof(buf), received_bytes) == Status::Ok) {
    Serial.printf("Received: %.*s\n", received_bytes, buf);
  }
}
```

# Installation

## PlatformIO

Add to your `platformio.ini`:

```ini
[env:your_env]
; Most recent changes
lib_deps =
  https://github.com/alkonosst/LoRa-E22.git

; Pinned release (recommended for production)
lib_deps =
  https://github.com/alkonosst/LoRa-E22.git#vx.y.z
```

## Arduino IDE

1. Open Arduino IDE.
2. Go to **Sketch > Manage Libraries...**
3. Search for **"LoRa-E22T"**.
4. Click **Install**.

# Usage

## Including the library

A single header includes all public types:

```cpp
#include <LoRa-E22T.h>
```

## Namespace

All public types live in the `E22` namespace. Add `using namespace E22;` to avoid repeating the prefix:

```cpp
using namespace E22;

static LoRaE22T lora;
static Model model = Model::E22_900T30;
```

## Initialization

Call `begin()` once in `setup()`. Initialize the `HardwareSerial` port before calling `begin()`. After a successful `begin()`, the module is placed in `Mode::Transmission`.

```cpp
Serial1.begin(9600, SERIAL_8N1, PIN_RX, PIN_TX);

Status s = lora.begin(
  Model::E22_900T30, // module model
  Serial1,           // initialized serial port
  1,                 // M0 pin
  2,                 // M1 pin
  3,                 // AUX pin
  -1                 // RESET pin (-1 = not connected)
);

if (s != Status::Ok) {
  Serial.printf("Init failed: %s\n", statusToString(s));
  while (true);
}
```

If the RESET pin is connected, call `reset()` at any time to perform a hardware reset:

```cpp
lora.reset(); // pulses RESET LOW, then waits for module startup
```

## Operating Modes

The module has four operating modes, switched via `setMode()`:

| Mode                  | Description                                                      |
| --------------------- | ---------------------------------------------------------------- |
| `Mode::Transmission`  | Default mode for sending/receiving data.                         |
| `Mode::WakeOnRadio`   | Low-power mode where the module wakes on radio activity.         |
| `Mode::Configuration` | Mode for reading/writing configuration registers.                |
| `Mode::DeepSleep`     | Lowest power mode; module is unresponsive until mode is changed. |

```cpp
lora.setMode(Mode::Configuration);
// ... configure registers ...
lora.setMode(Mode::Transmission);
```

> [!IMPORTANT]
> All `setXxx()` and `getXxx()` configuration methods require `Mode::Configuration`. All data send/receive methods require `Mode::Transmission` (or `Mode::WakeOnRadio` for WOR transmitters).

## Configuration

### Setters

Individual setters change one parameter at a time. They all require `Mode::Configuration`:

```cpp
lora.setMode(Mode::Configuration);
lora.setAddress(0x0001, false);
lora.setNetworkID(0x00, false);
lora.setChannel(18, false); // 900MHz models: 0-80 (frequency [MHz] = 850.125 + channel)
lora.setTransmissionMode(TxMode::Fixed, false);
lora.setAirDataRate400_900(AirDataRate400_900::Kbps2_4, false);
lora.setTransmissionPower30dBm(TxPower30dBm::dBm30, false);
lora.setEncryptionKey(0xABCD, false); // write-only; reads always return 0
lora.setMode(Mode::Transmission);
```

All setters have a `persistent` boolean parameter. See [Persistent vs temporary](#persistent-vs-temporary) below for details.

Full list of setters:

| Method                                         | Description                                                   |
| ---------------------------------------------- | ------------------------------------------------------------- |
| `setFactorySettings()`                         | Restore all registers to factory defaults.                    |
| `setConfig(config, persistent)`                | Write all registers at once.                                  |
| `setWirelessConfig(config, persistent)`        | Send a configuration command to a remote module over-the-air. |
| `setAddress(addr, persistent)`                 | 16-bit module address.                                        |
| `setNetworkID(id, persistent)`                 | Network ID (must match on all nodes in the same network).     |
| `setUARTConfig(baud, parity, persistent)`      | UART baud rate and parity.                                    |
| `setAirDataRate230(rate, persistent)`          | Air data rate, only for 230 MHz models.                       |
| `setAirDataRate400_900(rate, persistent)`      | Air data rate, only for 400/900 MHz models.                   |
| `setSubpacketLength(len, persistent)`          | Maximum subpacket size for fragmented transmissions.          |
| `setAmbientRSSI(enable, persistent)`           | Enable ambient noise RSSI reporting.                          |
| `setTransmissionPower22dBm(power, persistent)` | TX power, only for 22 dBm models.                             |
| `setTransmissionPower30dBm(power, persistent)` | TX power, only for 30 dBm models.                             |
| `setChannel(ch, persistent)`                   | RF channel number.                                            |
| `setPacketRSSI(enable, persistent)`            | Append RSSI byte to each received packet.                     |
| `setTransmissionMode(mode, persistent)`        | Transparent or Fixed.                                         |
| `setRelayMode(enable, persistent)`             | Enable relay (repeater) mode.                                 |
| `setListenBeforeTalk(enable, persistent)`      | Enable LBT (carrier sense before transmit).                   |
| `setWORMode(mode, persistent)`                 | WOR Receiver or Transmitter.                                  |
| `setWORCycleTime(cycle, persistent)`           | WOR wake cycle period (500 ms – 4000 ms).                     |
| `setEncryptionKey(key, persistent)`            | 16-bit hardware encryption key (0 = disabled).                |
| `setWORDelay(ms, persistent)`                  | WOR receiver response window delay in milliseconds.           |

### Getters

```cpp
lora.setMode(Mode::Configuration);

uint16_t addr;
lora.getAddress(addr);
Serial.printf("Address: 0x%04X\n", addr);

uint8_t ch;
lora.getChannel(ch);
Serial.printf("Channel: %u\n", ch);

lora.setMode(Mode::Transmission);
```

Full list of getters:

| Method                         | Description                                                |
| ------------------------------ | ---------------------------------------------------------- |
| `getConfig(config)`            | Read all registers at once into a `LoRaE22TConfig` struct. |
| `getWirelessConfig(config)`    | Request the remote module's full config over-the-air.      |
| `getAddress(addr)`             | 16-bit address.                                            |
| `getNetworkID(id)`             | Network ID.                                                |
| `getUARTConfig(baud, parity)`  | UART baud rate and parity.                                 |
| `getAirDataRate(raw)`          | Raw air data rate register value.                          |
| `getSubpacketLength(len)`      | Subpacket length.                                          |
| `getAmbientRSSI(enabled)`      | Whether ambient RSSI is enabled.                           |
| `getTransmissionPower(raw)`    | Raw TX power register value.                               |
| `getChannel(ch)`               | RF channel number.                                         |
| `getPacketRSSI(enabled)`       | Whether packet RSSI byte appending is enabled.             |
| `getTransmissionMode(mode)`    | Transparent or Fixed.                                      |
| `getRelayMode(enabled)`        | Whether relay mode is enabled.                             |
| `getListenBeforeTalk(enabled)` | Whether LBT is enabled.                                    |
| `getWORMode(mode)`             | WOR mode.                                                  |
| `getWORCycleTime(cycle)`       | WOR cycle time.                                            |
| `getWORDelay(ms)`              | WOR response window delay.                                 |

> [!NOTE]
> The encryption key register is **write-only** on the hardware. Reading it always returns `0x0000`. There is no `getEncryptionKey()` method.

### Bulk read/write

To read or write the entire configuration in a single UART transaction, use `getConfig()` and `setConfig()`:

```cpp
LoRaE22TConfig config;
lora.setMode(Mode::Configuration);
lora.getConfig(config);

// Modify only what you need
config.address = 0x0005;
config.channel = 0x12;
config.tx_mode = TxMode::Fixed;

lora.setConfig(config, false);
lora.setMode(Mode::Transmission);
```

To configure a remote module over-the-air without physical access, use `setWirelessConfig()` and `getWirelessConfig()`:

```cpp
// Send a new config to the remote module
LoRaE22TConfig remote;
remote.address = 0x0010;
remote.channel = 0x15;
lora.setMode(Mode::Configuration);
lora.setWirelessConfig(remote, false);
lora.setMode(Mode::Transmission);

// Query the current config of the remote module
LoRaE22TConfig queried;
lora.setMode(Mode::Configuration);
lora.getWirelessConfig(queried);
lora.setMode(Mode::Transmission);
Serial.printf("Remote channel: 0x%02X\n", queried.channel);
```

### Persistent vs temporary

Every setter accepts a `persistent` boolean as the last parameter:

- `false` (default): setting is applied immediately and lost after a power cycle or hardware reset.
- `true`: setting is written to the module's flash and survives power cycles.

> [!CAUTION]
> Flash has a limited write endurance. Use `persistent = false` during development and testing to avoid unnecessary flash wear.

### Channel frequencies

The RF channel number maps to a frequency depending on the module band:

| Band    | Channel range | Frequency formula       |
| ------- | ------------- | ----------------------- |
| 230 MHz | 0–64          | 220.125 + ch × 0.25 MHz |
| 400 MHz | 0–83          | 410.125 + ch MHz        |
| 900 MHz | 0–80          | 850.125 + ch MHz        |

Factory default channels:

| Band    | Channel   | Frequency   |
| ------- | --------- | ----------- |
| 230 MHz | 40 (0x28) | 230.125 MHz |
| 400 MHz | 23 (0x17) | 433.125 MHz |
| 900 MHz | 18 (0x12) | 868.125 MHz |

## Data Transmission

All send methods require the module to be in `Mode::Transmission` (or `Mode::WakeOnRadio` for WOR transmitters). They block until the module signals AUX HIGH, confirming the packet has been handed off to the RF layer.

### Transparent mode

All modules sharing the same address and channel receive every packet. No address header is added.

```cpp
const char* msg = "Hello!";
Status s = lora.sendTransparentData(
  reinterpret_cast<const uint8_t*>(msg), strlen(msg));
```

### Fixed mode

Each packet carries a destination 16-bit address and channel. Only the module with the matching address and channel receives it. Use address `0xFFFF` (via `sendBroadcastFixedData`) to reach all modules on a channel.

```cpp
// Unicast to address 0x0002, channel 0x12
Status s = lora.sendFixedData(
  0x0002, 0x12,
  reinterpret_cast<const uint8_t*>(msg), strlen(msg));

// Broadcast to all modules on channel 0x12
Status s = lora.sendBroadcastFixedData(
  0x12,
  reinterpret_cast<const uint8_t*>(msg), strlen(msg));
```

### Wake-on-Radio (WOR)

WOR allows a receiver to sleep between periodic wake windows, dramatically reducing average power consumption. The transmitter must send a preamble at least as long as the WOR cycle before the payload so the receiver wakes in time.

**Receiver setup:**

```cpp
lora.setMode(Mode::Configuration);
lora.setWORMode(WORMode::Receiver, false);
lora.setWORCycleTime(WORCycleTime::Ms2000, false);
lora.setMode(Mode::WakeOnRadio);  // enter low-power WOR mode
```

**Transmitter setup:**

```cpp
lora.setMode(Mode::Configuration);
lora.setWORMode(WORMode::Transmitter, false);
lora.setWORCycleTime(WORCycleTime::Ms2000, false); // must match receiver
lora.setMode(Mode::WakeOnRadio);

// Use the dedicated WOR send methods
Status s = lora.sendWORTransparentData(...);
Status s = lora.sendWORFixedData(address, channel, ...);
Status s = lora.sendWORBroadcastFixedData(channel, ...);
```

> [!IMPORTANT]
> Always use the `sendWORxxx()` variants when operating as a WOR transmitter. The standard `sendxxx()` methods use a 500 ms AUX timeout, which is shorter than the maximum WOR preamble duration (up to 4000 ms for the longest cycle time).

## Data Reception

### Reading data

Poll `isDataAvailable()` in `loop()` and call `readData()` when data arrives:

```cpp
static uint8_t buf[256];

void loop() {
  if (!lora.isDataAvailable()) return;

  size_t n = 0;
  Status s = lora.readData(false, buf, sizeof(buf), n);
  if (s == Status::Ok) {
    // process buf[0..n-1]
  }
}
```

The first parameter `includes_rssi` tells the library whether to strip and internally store the trailing RSSI byte appended by the module (only relevant when `setPacketRSSI(true)` is active):

```cpp
// Pass true to strip the RSSI byte; retrieve it afterwards with getLastPacketRSSI()
lora.readData(true, buf, sizeof(buf), n);
```

### Packet RSSI

Enable per-packet RSSI to measure the received signal strength of each individual packet:

```cpp
lora.setMode(Mode::Configuration);
lora.setPacketRSSI(true, false);
lora.setMode(Mode::Transmission);

// In loop():
lora.readData(true, buf, sizeof(buf), n);
int16_t rssi = lora.getLastPacketRSSI(); // dBm (negative); closer to 0 = stronger signal
```

### Ambient RSSI

Read the channel noise floor at any time to assess RF conditions without waiting for a packet:

```cpp
lora.setMode(Mode::Configuration);
lora.setAmbientRSSI(true, false);
lora.setMode(Mode::Transmission);

// In loop():
int16_t noise;
lora.readAmbientRSSI(noise);
Serial.printf("Noise floor: %d dBm\n", noise);
```

## Error Handling

Every method returns a `Status` value. Use `statusToString()` to convert it to a printable C string:

```cpp
Status s = lora.setAddress(0x0001, false);
if (s != Status::Ok) {
  Serial.printf("Error: %s\n", statusToString(s));
}
```

| Status               | Description                                                     |
| -------------------- | --------------------------------------------------------------- |
| `Ok`                 | Operation completed successfully.                               |
| `ModelNotSet`        | No model was provided to `begin()`.                             |
| `SerialNotSet`       | No serial port was provided to `begin()`.                       |
| `PinsNotSet`         | Required control pins were not configured.                      |
| `Uninitialized`      | `begin()` has not been called.                                  |
| `AlreadyInitialized` | `begin()` was called more than once.                            |
| `AuxTimeout`         | AUX pin did not return HIGH within the timeout window.          |
| `SerialTimeout`      | Module did not respond to a serial command in time.             |
| `WrongMode`          | Method called while the module was in an incompatible mode.     |
| `WrongModel`         | Method called on a model that does not support the operation.   |
| `InvalidParameter`   | A parameter value was out of range or invalid.                  |
| `CommandFailed`      | Module returned an unexpected or error response.                |
| `BufferTooSmall`     | The provided receive buffer is too small for the incoming data. |

## Logging

Logging is **disabled by default** and has zero runtime cost when turned off - all macros expand to `void(0)`.

To enable it, define `LORA_E22T_LOG_LEVEL` to a value between 1 and 5 before the library is compiled:

| Level | Macro                               | Typical use                              |
| ----- | ----------------------------------- | ---------------------------------------- |
| 1     | `LORA_E22T_LOGE`                    | Errors (timeouts, invalid parameters...) |
| 2     | `LORA_E22T_LOGW`                    | Warnings                                 |
| 3     | `LORA_E22T_LOGI`                    | Informational events                     |
| 4     | `LORA_E22T_LOGD`                    | Debug (function entry, byte counts...)   |
| 5     | `LORA_E22T_LOGV` / `LORA_E22T_HEXV` | Verbose + hex dumps                      |

Each level includes all levels below it (e.g. level 4 also emits errors, warnings, and info).

**PlatformIO** - set in `platformio.ini`:

```ini
[env:your_env]
build_flags =
  -D LORA_E22T_LOG_LEVEL=4
```

**Arduino IDE** - define before including the library:

```cpp
#define LORA_E22T_LOG_LEVEL 4
#include <LoRa-E22T.h>
```

### Configuration macros

| Macro                      | Default        | Description                                                                                                                          |
| -------------------------- | -------------- | ------------------------------------------------------------------------------------------------------------------------------------ |
| `LORA_E22T_LOG_LEVEL`      | `0` (disabled) | Active log level (0-5).                                                                                                              |
| `LORA_E22T_LOG_SERIAL`     | `Serial`       | Serial port used when not on ESP32 (or when ESP32 logs are off).                                                                     |
| `LORA_E22T_LOG_TAG`        | `"LoRaE22T"`   | Tag string prepended to every log line.                                                                                              |
| `LORA_E22T_USE_ESP32_LOGS` | `1` (enabled)  | When `1` on ESP32, routes logs through `ESP_LOGx()` instead of `Serial.printf()`. Set to `0` to use `Serial.printf()` even on ESP32. |

# Release Status

This project is in active development. Until reaching version **v1.0.0**, consider it **beta
software**. APIs may change in future releases, and some features may be incomplete or unstable.
Please report any issues on the [GitHub Issues](https://github.com/alkonosst/LoRa-E22T/issues) page.

# License

This project is licensed under the [MIT License](LICENSE).
