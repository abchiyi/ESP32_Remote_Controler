#include <Arduino.h>
#include <esp_log.h>

// 无线依赖
#include <esp_now.h>
#include <esp_wifi.h>
#include <radio.h>
#include <WiFi.h>

// 控制器依赖
#include <controller.h>
#include <XBOXONE.h>

// 显示
// #include <display.h>

// gui
#include <U8g2lib.h>
#include <WouoUI.h>
// page
#include <view/fidgetToy.h>
#include <view/setting.h>
#include <view/editor.h>
#include <view/window.h>
#include <view/about.h>
#include <view/sleep.h>
#include <view/menu.h>
#include <view/mainPage.h>

#define TAG "Main ESP32 RC"

// 显示器引脚
#define SCL 22
#define SDA 21

#define RES 32
#define DC 33
#define CS 25

Radio radio;
Controller controller;

// Display display;

U8G2_SSD1306_128X64_NONAME_F_4W_HW_SPI u8g2(U8G2_R0, CS, DC, RES);

WouoUI wouoUI(&u8g2);

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
  auto ui = gui->ui;

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
        if (btn.count > ui.param[BTN_LPT])
          break;
        delay(1);
      }
      *LPT_flag = (btn.count < ui.param[BTN_LPT]) ? 0 : 1;
      gui->btnPressed = true;
    }
    else
    {
      gui->btnPressed = 1;
      vTaskDelay(ui.param[BTN_SPT]);
    }
  }
}

void btn_scan(WouoUI *gui)
{
  auto ui = gui->ui;

  uint8_t *btnA = (uint8_t *)&controller.data.btnA;
  uint8_t *btnB = (uint8_t *)&controller.data.btnB;

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

  uint8_t *btnUp = (uint8_t *)&controller.data.btnDirUp;
  uint8_t *btnDown = (uint8_t *)&controller.data.btnDirDown;
  uint8_t joyLUp = controller.data.joyLVert > 300 ? 1 : 0;
  uint8_t joyLDown = controller.data.joyLVert < -300 ? 1 : 0;
  uint8_t btnMenu = controller.data.btnStart;

  singleBtnScan(&wouoUI, btnUp, (uint8_t *)&btnUpLPT, BTN_ID_UP);
  singleBtnScan(&wouoUI, btnDown, (uint8_t *)&btnDownLPT, BTN_ID_DO);

  singleBtnScan(&wouoUI, &joyLUp, (uint8_t *)&joyUPLPT, BTN_ID_UP);
  singleBtnScan(&wouoUI, &joyLDown, (uint8_t *)&joyDOWNLPT, BTN_ID_DO);

  singleBtnScan(&wouoUI, &btnMenu, (uint8_t *)&btnMenuLPT, BTN_ID_MENU);
}

// 数据层处理任务
void TaskDataLayerUpdate(void *pt)
{
  while (true)
  {
    wouoUI.btnUpdate(btn_scan);
    if (!wouoUI.btnPressed) // 没有按钮被触发时执行
      vTaskDelay(1);
  }
}

// WouoUI *wouo_ui = display

esp_err_t
sendCB(uint8_t *peer_addr)
{
  return esp_now_send(peer_addr,
                      (uint8_t *)&controller.data,
                      sizeof(controller.data));
}

void setup()
{
  Serial.begin(115200);
  controller.begin();
  delay(1000); // 延迟启动无线连接
  radio.begin(sendCB, 10);
  // display.begin(&radio);

  // 注册页面
  wouoUI.addPage(P_MENU);
  wouoUI.addPage(P_EDITOR);
  wouoUI.addPage(P_SETTING);
  wouoUI.addPage(P_WINDOW);
  wouoUI.addPage(P_ABOUT);
  wouoUI.addPage(F0TOY);
  wouoUI.addPage(P_SLEEP);
  wouoUI.addPage(P_MAIN);

  wouoUI.setDefaultPage(P_MAIN);

  wouoUI.begin();

  // 设置数据层更新任务
  xTaskCreatePinnedToCore(
      TaskDataLayerUpdate,
      "WouoUI datat update",
      1024 * 8, (void *)&wouoUI, 1, NULL, 0);
}

void loop()
{
}