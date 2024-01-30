#include <Arduino.h>
#include <esp_log.h>

// // 显示依赖
// #include <SPI.h>
// #include <Wire.h>
// #include <Adafruit_GFX.h>
// #include <Adafruit_SSD1306.h>

// 无线依赖
#include <esp_now.h>
#include <esp_wifi.h>
#include <radio.h>
#include <WiFi.h>

// 控制器依赖
#include <controller.h>

/* ESP32 WROOM-32E */

// 主显示器 Spi：
#define DO 14
#define DI 13
#define DC 26
#define RES 25
#define CS 27
// 副显示器 I2C
#define SCK 22
#define SDA 21

#define TAG "Main ESP32 RC"

Radio radio;

Controller controller;

void setup()
{
  Serial.begin(115200);
  // radio.begin(&controller, 100);
  controller.begin();
}

void loop()
{
  // if (controller.getConnectStatus())
  // {
  // ESP_LOGI(TAG, "Controller : \n %s", controller.toString().c_str());
  Serial.println("\r" + controller.toString());
  // }
  delay(100);
}