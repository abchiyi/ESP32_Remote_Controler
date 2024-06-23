#include <Arduino.h>
#include <variant>
#include <esp_log.h>
#include "EEPROM.h"

// 无线依赖
#include <esp_now.h>
#include <esp_wifi.h>
#include <radio.h>
#include <WiFi.h>

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
#include "view/setting_devces.h"
#include <view/fidgetToy.h>
#include <view/mainPage.h>
#include <view/setting.h>
#include <view/window.h>
#include <view/sleep.h>
#include <view/about.h>
#include <view/menu.h>

#define TAG "Main ESP32 RC"

// 显示器引脚
#define SCL 22
#define SDA 21

#define RES 6
#define DC 5
#define CS 4

// Screen
U8G2_SSD1306_128X64_NONAME_F_4W_HW_SPI u8g2(U8G2_R0, CS, DC, RES);
WouoUI WOUO_UI(&u8g2); // 定义gui对象

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

void singleBtnScan(WouoUI *gui, uint8_t *s_btn, uint8_t *LPT_flag, uint8_t active_value)
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
    gui->btnID = active_value;
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
      gui->btnPressed = true;
    }
    else
    {
      gui->btnPressed = 1;
      vTaskDelay(btn_SPT);
    }
  }
}

void btn_scan(WouoUI *gui)
{
  uint8_t *btnA = (uint8_t *)&Controller.btnA;
  uint8_t *btnB = (uint8_t *)&Controller.btnB;

  if (Xbox.getButtonClick(A))
  {
    gui->btnID = BTN_ID_CONFIRM;
    gui->btnPressed = true;
    btn.count = 0;
  }

  if (Xbox.getButtonClick(B))
  {
    gui->btnID = BTN_ID_CANCEL;
    gui->btnPressed = true;
    btn.count = 0;
  }

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

  singleBtnScan(&WOUO_UI, btnUp, (uint8_t *)&btnUpLPT, BTN_ID_UP);
  singleBtnScan(&WOUO_UI, btnDown, (uint8_t *)&btnDownLPT, BTN_ID_DO);

  singleBtnScan(&WOUO_UI, &joyLUp, (uint8_t *)&joyUPLPT, BTN_ID_UP);
  singleBtnScan(&WOUO_UI, &joyLDown, (uint8_t *)&joyDOWNLPT, BTN_ID_DO);

  singleBtnScan(&WOUO_UI, &btnMenu, (uint8_t *)&btnMenuLPT, BTN_ID_MENU);
}

// 数据层处理任务
void TaskDataLayerUpdate(void *pt)
{
  while (true)
  {
    WOUO_UI.btnUpdate(btn_scan);
    if (!WOUO_UI.btnPressed) // 没有按钮被触发时执行
      vTaskDelay(1);
  }
}

void ISR()
{
  if (RADIO.status == RADIO_CONNECTED)
    RADIO.status = RADIO_BEFORE_DISCONNECT;
  else
    RADIO.status = RADIO_PAIR_DEVICE;
};

void setup()
{
  Serial.begin(115200);

  /** 控制器输入 **/
  Controller.begin();

  /** 控制输出 **/
  car_controll_start();

  /** GUI **/
  WOUO_UI.setDefaultPage(create_page_main);
  // WOUO_UI.setDefaultPage(create_page_setting);
  WOUO_UI.begin(&u8g2);

  /** 无线 **/
  RADIO.begin();

  // 设置数据层更新任务
  xTaskCreatePinnedToCore(TaskDataLayerUpdate, "WouoUI datat update",
                          1024 * 8, NULL, 1, NULL, 0);

  // 无线扫描按钮
  pinMode(0, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(0), ISR, RISING);

  vTaskDelete(NULL); // 干掉 loopTask
}

void loop()
{
}
