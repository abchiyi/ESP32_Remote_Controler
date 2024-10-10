#include <controller.h>
#include <XBOXONE.h>
#include <esp_log.h>
#include <Usb.h>
#include <radio.h>
#include "FreeRTOS.h"

#include "pins.h"

#define TAG "Controller"
USB Usb;
XBOXONE Xbox(&Usb);
CONTROLLER Controller;

// 初始化连接手柄外设usb
void usbInit()
{
  ESP_LOGI(TAG, "Usb init");
  pinMode(PIN_USB_HOST_SHELL_REST, OUTPUT);
  digitalWrite(PIN_USB_HOST_SHELL_REST, HIGH);

  static int counter = 0;
  if (Usb.Init() == -1)
  {
    ESP_LOGE(TAG, "OSC did not start");
    // 重试5次初始化usb失败后重启mcu
    delay(10);
    counter++;
    counter < 5 ? usbInit() : ESP.restart();
  }
  else
  {
    counter = 0; // 成功初始化usb重置计数器
    ESP_LOGI(TAG, "XBOX USB Library Started");
  }
}

// 使用独立任务避免usb数据读取卡死
void task_update(void *pt)
{
  usbInit();
  TickType_t xLastWakeTime = xTaskGetTickCount(); // 最后唤醒时间
  const TickType_t xFrequency = pdMS_TO_TICKS(8); // 设置采样率 250hz
  while (true)
  {
    Usb.Task();
    Controller.update();
    xTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

// 0 ~ 2047
const int deadZone = 8000;
const int maxLength = 32768;
const int resolution = 2048;
const float_t step = (float_t)(maxLength - deadZone) / (float_t)resolution;
void syncAnalogHat(int16_t _from, int16_t *_to)
{
  // 计算除去死区后摇杆的值
  const int16_t t_from = _from < 0 ? _from - (deadZone * -1) : _from - deadZone;

  int16_t toValue = _from < (deadZone * -1) || _from > deadZone
                        ? (float_t)t_from / step
                        : 0;

  *_to = toValue < -2048 ? -2048 : toValue;
}

// 启动xbox控制器
void CONTROLLER::begin()
{
  xTaskCreatePinnedToCore(task_update, "taskUSB", 4096, NULL, configMAX_PRIORITIES - 1, NULL, 0);
}

void CONTROLLER::update()
{
  if (Xbox.XboxOneConnected)
  {
    // 字母键
    btnA = Xbox.getButtonPress(A);
    btnB = Xbox.getButtonPress(B);
    btnX = Xbox.getButtonPress(X);
    btnY = Xbox.getButtonPress(Y);

    // 功能键
    btnStart = Xbox.getButtonPress(START);
    // btnXbox = Xbox.getButtonPress(XBOX); // XXX 功能异常不使用
    btnSelect = Xbox.getButtonPress(BACK);
    btnShare = Xbox.getButtonPress(SYNC);

    // 肩键
    btnLB = Xbox.getButtonPress(L1);
    btnRB = Xbox.getButtonPress(R1);

    // 方向键
    btnDirUp = Xbox.getButtonPress(UP);
    btnDirDown = Xbox.getButtonPress(DOWN);
    btnDirLeft = Xbox.getButtonPress(LEFT);
    btnDirRight = Xbox.getButtonPress(RIGHT);

    // 摇杆
    syncAnalogHat(Xbox.getAnalogHat(LeftHatX), &joyLHori);
    syncAnalogHat(Xbox.getAnalogHat(LeftHatY), &joyLVert);
    btnLS = Xbox.getButtonPress(L3);

    syncAnalogHat(Xbox.getAnalogHat(RightHatX), &joyRHori);
    syncAnalogHat(Xbox.getAnalogHat(RightHatY), &joyRVert);
    btnRS = Xbox.getButtonPress(R3);

    // 扳机键
    trigLT = Xbox.getButtonPress(L2);
    trigRT = Xbox.getButtonPress(R2);
  }
};

// ↓this code from XboxControllerNotificationParser lib
String CONTROLLER::toString()
{
  // clang-format off
  String str = String("") +
    "btnY: " + String(btnY) + " " +
    "btnX: " + String(btnX) + " " +
    "btnB: " + String(btnB) + " " +
    "btnA: " + String(btnA) + " " +
    "btnLB: " + String(btnLB) + " " +
    "btnRB: " + String(btnRB) + "\n" +
    "btnSelect: " + String(btnSelect) + " " +
    "btnStart: " + String(btnStart) + " " +
    "btnXbox: " + String(btnXbox) + " " +
    "btnShare: " + String(btnShare) + " " +
    "btnLS: " + String(btnLS) + " " +
    "btnRS: " + String(btnRS) + "\n" +
    "btnDirUp: " + String(btnDirUp) + " " +
    "btnDirRight: " + String(btnDirRight) + " " +
    "btnDirDown: " + String(btnDirDown) + " " +
    "btnDirLeft: " + String(btnDirLeft) + "\n"
    "joyLHori: " + String(joyLHori) + "\n" +
    "joyLVert: " + String(joyLVert) + "\n" +
    "joyRHori: " + String(joyRHori) + "\n" +
    "joyRVert: " + String(joyRVert) + "\n" +
    "trigLT: " + String(trigLT) + "\n" +
    "trigRT: " + String(trigRT) + "\n";
  return str;
}