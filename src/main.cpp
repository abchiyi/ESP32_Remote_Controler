#include <Arduino.h>
#include <variant>
#include <esp_log.h>
#include "EEPROM.h"
#include "pins_arduino.h"
// 无线依赖
#include <radio.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// 控制器依赖
#include <controller.h>
#include <pins_arduino.h>
#include "HID.h"

// 车辆控制
#include "car.h"

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

// 显示器引脚
#define SCL 22
#define SDA 21

#define RES 6
#define DC 5
#define CS 4

// Screen
// U8G2_SSD1306_128X64_NONAME_F_4W_HW_SPI u8g2(U8G2_R0, CS, DC, RES);
U8G2_SH1107_PIMORONI_128X128_F_4W_HW_SPI u8g2(U8G2_R3, CS, DC, RES);

// 按键变量
struct
{
  uint8_t id;
  bool flag;
  bool pressed;
  bool CW_1;
  bool CW_2;
  bool val;
  bool val_last;
  bool alv;
  bool blv;
  long count;
} volatile btn;

void ISR()
{
  // RADIO.status = RADIO_PAIR_DEVICE;
  RADIO.status = RADIO_IN_SCAN_BEFORE;
};

void setup()
{
  Serial.begin(115200);
  vTaskDelay(500);
  Power.begin();

  /** 控制器输入 **/
  Controller.begin();

  /** 控制输出 **/
  // car_control_start();

  hid_begin();

  /** GUI **/
  WOUO_UI.begin(&u8g2);

  WOUO_UI.setDefaultPage(create_page_main);
  u8g2.setContrast(255);

  /** 无线 **/
  RADIO.begin();

  RADIO.cb_fn_after_recv = [&](radio_packet_t data)
  {
    read_power_info(&data);
  };

  // 无线扫描按钮
  // pinMode(0, INPUT_PULLUP);
  // attachInterrupt(digitalPinToInterrupt(0), ISR, RISING);

  // vTaskDelete(NULL); // 干掉 loopTask
}

void loop()
{
}
