#include <Arduino.h>
#include <variant>
#include <esp_log.h>
#include "EEPROM.h"
#include "pins_arduino.h"
// 无线依赖
#include "wifi_link.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// 控制器依赖
#include <controller.h>
#include <pins_arduino.h>
#include "HID.h"

// 显示
// gui
#include <U8g2lib.h>
#include <WouoUI.h>
// page
#include "view/mainPage.h"
#include "view/setting.h"
#include "pins_arduino.h"

#define TAG "Main ESP32 RC"

// 电源
#include "power.h"

// // 显示器引脚
// #define SCL 22
// #define SDA 21

#define RES 6
#define DC 5
#define CS 4

// Screen
// U8G2_SSD1306_128X32_NONAME_F_4W_HW_SPI u8g2(U8G2_R0, CS, DC, RES);
// U8G2_SH1107_PIMORONI_128X128_F_4W_HW_SPI u8g2(U8G2_R3, CS, DC, RES);

// U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, SCL, SDA);
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, SCL, SDA);

void setup()
{
  Serial.begin(115200);
  // Power.begin();
  // vTaskDelay(100);

  start_wifi_link();
  vTaskDelay(100);

  /** 控制器输入 **/
  Controller.begin();
  hid_begin();
  vTaskDelay(100);

  /** GUI **/
  WOUO_UI.begin(&u8g2);
  WOUO_UI.setDefaultPage(create_page_main);
  u8g2.setContrast(255);

  vTaskDelete(NULL); // 干掉 loopTask
}

void loop()
{
}
