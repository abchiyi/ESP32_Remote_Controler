
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <esp_now.h>
#include <esp_wifi.h>
#include <radio.h>
#include <WiFi.h>

#include <esp_log.h>

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

int data = 1;
void setup()
{
  radio.begin(&data, 100);
}

void loop()
{
  data = 0;
  for (int i = 0; i <= 255; i++)
  {
    data++;
    delay(100);
    ESP_LOGI(TAG, "Data : %u", data);
  }
}