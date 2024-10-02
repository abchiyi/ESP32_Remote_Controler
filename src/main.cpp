#include <Arduino.h>
#include <variant>
#include <esp_log.h>
#include "EEPROM.h"
#include "pins_arduino.h"
// 无线依赖
#include <esp_now.h>
#include <esp_wifi.h>
#include <radio.h>
#include <WiFi.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// 控制器依赖
#include <controller.h>
#include <pins_arduino.h>

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

#define RES 4
#define DC 5
#define CS 6

// Screen
// U8G2_SSD1306_128X64_NONAME_F_4W_HW_SPI u8g2(U8G2_R0, CS, DC, RES);
U8G2_SH1107_PIMORONI_128X128_F_4W_HW_SPI u8g2(U8G2_R2, CS, DC, RES);

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

void singleBtnScan(WouoUI *gui,
                   uint8_t *s_btn,
                   uint8_t *LPT_flag,
                   key_id_t key)
{
  uint8_t btn_LPT = 255;
  uint8_t btn_SPT = 50;

  btn_LPT = CONFIG_UI[BTN_LPT];
  btn_SPT = CONFIG_UI[BTN_SPT];

  if (!*s_btn && LPT_flag) // 按钮没有被按下结束程序
  {
    *LPT_flag = false;
    return;
  }

  if (*s_btn)
  {
    if (!*LPT_flag) // 长按激活时不再进入长按计算块
    {
      btn.count = 0;
      while (*s_btn) // 计算按压时间
      {
        btn.count++;
        if (btn.count > btn_LPT)
          break;
        delay(1);
      }
      *LPT_flag = (btn.count < btn_LPT) ? 0 : 1;
      gui->dispatch_event(Event(key));
    }
    else
    {
      gui->dispatch_event(Event(key));
      vTaskDelay(btn_SPT);
    }
  }
}

void btn_scan(WouoUI *gui)
{
  uint8_t *btnA = (uint8_t *)&Controller.btnA;
  uint8_t *btnB = (uint8_t *)&Controller.btnB;

  if (Xbox.getButtonClick(A))
    gui->dispatch_event(Event(KEY_CONFIRM));

  if (Xbox.getButtonClick(B))
    gui->dispatch_event(Event(KEY_BACK));

  static bool btnUpLPT = false;
  static bool btnDownLPT = false;
  static bool joyUPLPT = false;
  static bool joyDOWNLPT = false;
  static bool btnMenuLPT = false;

  uint8_t *btnUp = (uint8_t *)&Controller.btnDirUp;
  uint8_t *btnDown = (uint8_t *)&Controller.btnDirDown;
  uint8_t joyLUp = Controller.joyLVert > 300 ? 1 : 0;
  uint8_t joyLDown = Controller.joyLVert < -300 ? 1 : 0;
  uint8_t btnMenu = Controller.btnStart;

  singleBtnScan(&WOUO_UI, btnUp, (uint8_t *)&btnUpLPT, KEY_UP);
  singleBtnScan(&WOUO_UI, btnDown, (uint8_t *)&btnDownLPT, KEY_DOWN);

  singleBtnScan(&WOUO_UI, &joyLUp, (uint8_t *)&joyUPLPT, KEY_UP);
  singleBtnScan(&WOUO_UI, &joyLDown, (uint8_t *)&joyDOWNLPT, KEY_DOWN);

  singleBtnScan(&WOUO_UI, &btnMenu, (uint8_t *)&btnMenuLPT, KEY_MENU);
}

// 数据层处理任务
void TaskDataLayerUpdate(void *pt)
{
  while (true)
  {
    btn_scan(&WOUO_UI);
    vTaskDelay(5); // XXX 使用固定频率执行
  }
}

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
  car_controll_start();

  /** GUI **/
  WOUO_UI.begin(&u8g2);

  WOUO_UI.setDefaultPage(create_page_main);
  u8g2.setContrast(255);

  /** 无线 **/
  RADIO.begin();

  RADIO.cb_fn_arfter_recve = [&](radio_data_t data)
  {
    read_power_info(&data);
  };

  // 设置数据层更新任务
  xTaskCreatePinnedToCore(TaskDataLayerUpdate, "WouoUI data update",
                          1024 * 8, NULL, 1, NULL, 0);

  // 无线扫描按钮
  // pinMode(0, INPUT_PULLUP);
  // attachInterrupt(digitalPinToInterrupt(0), ISR, RISING);

  vTaskDelete(NULL); // 干掉 loopTask
}

void loop()
{
}
