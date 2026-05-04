#pragma once

#ifdef ESP32
#  include <esp_log.h>
#else
#  include <HardwareSerial.h>
#endif

// Log levels: 0=None, 1=Error, 2=Warning, 3=Info, 4=Debug, 5=Verbose
#ifndef LORA_E22T_LOG_LEVEL
#  define LORA_E22T_LOG_LEVEL 0
#endif

// Log output, default: Serial
#ifndef LORA_E22T_LOG_SERIAL
#  define LORA_E22T_LOG_SERIAL Serial
#endif

// Log tag
#ifndef LORA_E22T_LOG_TAG
#  define LORA_E22T_LOG_TAG "LoRaE22T"
#endif

// Use ESP32 logging function when using ESP32, default is enabled
#ifndef LORA_E22T_USE_ESP32_LOGS
#  define LORA_E22T_USE_ESP32_LOGS 1
#endif

// ESP32 logs
#if defined(ESP32) && LORA_E22T_USE_ESP32_LOGS

// Error
#  if LORA_E22T_LOG_LEVEL >= 1
#    define LORA_E22T_LOGE(format, ...) ESP_LOGE(LORA_E22T_LOG_TAG, format, ##__VA_ARGS__)
#  else
#    define LORA_E22T_LOGE(format, ...) void(0)
#  endif

// Warning
#  if LORA_E22T_LOG_LEVEL >= 2
#    define LORA_E22T_LOGW(format, ...) ESP_LOGW(LORA_E22T_LOG_TAG, format, ##__VA_ARGS__)
#  else
#    define LORA_E22T_LOGW(format, ...) void(0)
#  endif

// Info
#  if LORA_E22T_LOG_LEVEL >= 3
#    define LORA_E22T_LOGI(format, ...) ESP_LOGI(LORA_E22T_LOG_TAG, format, ##__VA_ARGS__)
#  else
#    define LORA_E22T_LOGI(format, ...) void(0)
#  endif

// Debug
#  if LORA_E22T_LOG_LEVEL >= 4
#    define LORA_E22T_LOGD(format, ...) ESP_LOGD(LORA_E22T_LOG_TAG, format, ##__VA_ARGS__)
#  else
#    define LORA_E22T_LOGD(format, ...) void(0)
#  endif

// Verbose
#  if LORA_E22T_LOG_LEVEL >= 5
#    define LORA_E22T_LOGV(format, ...) ESP_LOGV(LORA_E22T_LOG_TAG, format, ##__VA_ARGS__)
#    define LORA_E22T_HEXV(data, len)                                            \
      do {                                                                       \
        ESP_LOGV(LORA_E22T_LOG_TAG, "hexdump (len=%d):", len);                   \
        ESP_LOG_BUFFER_HEX_LEVEL(LORA_E22T_LOG_TAG, data, len, ESP_LOG_VERBOSE); \
      } while (0)
#  else
#    define LORA_E22T_LOGV(format, ...) void(0)
#    define LORA_E22T_HEXV(data, len)   void(0)

#  endif

// Other platforms: use printf
#else

// Error
#  if LORA_E22T_LOG_LEVEL >= 1
#    define LORA_E22T_LOGE(format, ...) \
      LORA_E22T_LOG_SERIAL.printf("E %s: " format "\r\n", LORA_E22T_LOG_TAG, ##__VA_ARGS__)
#  else
#    define LORA_E22T_LOGE(format, ...) void(0)
#  endif

// Warning
#  if LORA_E22T_LOG_LEVEL >= 2
#    define LORA_E22T_LOGW(format, ...) \
      LORA_E22T_LOG_SERIAL.printf("W %s: " format "\r\n", LORA_E22T_LOG_TAG, ##__VA_ARGS__)
#  else
#    define LORA_E22T_LOGW(format, ...) void(0)
#  endif

// Info
#  if LORA_E22T_LOG_LEVEL >= 3
#    define LORA_E22T_LOGI(format, ...) \
      LORA_E22T_LOG_SERIAL.printf("I %s: " format "\r\n", LORA_E22T_LOG_TAG, ##__VA_ARGS__)
#  else
#    define LORA_E22T_LOGI(format, ...) void(0)
#  endif

// Debug
#  if LORA_E22T_LOG_LEVEL >= 4
#    define LORA_E22T_LOGD(format, ...) \
      LORA_E22T_LOG_SERIAL.printf("D %s: " format "\r\n", LORA_E22T_LOG_TAG, ##__VA_ARGS__)
#  else
#    define LORA_E22T_LOGD(format, ...) void(0)
#  endif

// Verbose
#  if LORA_E22T_LOG_LEVEL >= 5
#    define LORA_E22T_LOGV(format, ...) \
      LORA_E22T_LOG_SERIAL.printf("V %s: " format "\r\n", LORA_E22T_LOG_TAG, ##__VA_ARGS__)
#    define LORA_E22T_HEXV(data, len)                                                       \
      do {                                                                                  \
        LORA_E22T_LOG_SERIAL.printf("V %s: hexdump (len=%d):\r\n", LORA_E22T_LOG_TAG, len); \
        for (size_t i = 0; i < len; i++) {                                                  \
          LORA_E22T_LOG_SERIAL.printf("%02X ", data[i]);                                    \
          if ((i + 1) % 16 == 0) LORA_E22T_LOG_SERIAL.println();                            \
        }                                                                                   \
        if (len % 16 != 0) LORA_E22T_LOG_SERIAL.println();                                  \
      } while (0)
#  else
#    define LORA_E22T_LOGV(format, ...) void(0)
#    define LORA_E22T_HEXV(data, len)   void(0)
#  endif

#endif
