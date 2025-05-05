#ifndef CONFIG_PINS
#define CONFIG_PINS
#include <Arduino.h>
#include <pins_arduino.h>

#ifdef CONFIG_IDF_TARGET_ESP32C3
#define BOOT_PIN 9
#else
#define BOOT_PIN 0 // 未指定io的 默认使用0号引脚
#endif

#endif
