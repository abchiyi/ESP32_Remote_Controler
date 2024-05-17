#include <controller.h>
#include <XBOXONE.h>
#include <esp_log.h>
#include <Usb.h>
#include <radio.h>
#include "FreeRTOS.h"

#define TAG "Controller"

bool *BtnA, *BtnB, *BtnX, *BtnY;
bool *BtnShare, *BtnStart, *BtnSelect, *BtnXbox;
bool *BtnLB, *BtnRB;
bool *BtnLS, *BtnRS;
bool *BtnDirUp, *BtnDirLeft, *BtnDirRight, *BtnDirDown;
int16_t *JoyLHori;
int16_t *JoyLVert;
int16_t *JoyRHori;
int16_t *JoyRVert;
int16_t *TrigLT, *TrigRT;

#define UsbHostShellRestPin 16

USB Usb;
XBOXONE Xbox(&Usb);

// 初始化连接手柄外设usb
void usbInit()
{
  ESP_LOGI(TAG, "Usb init");
  pinMode(UsbHostShellRestPin, OUTPUT);
  digitalWrite(UsbHostShellRestPin, HIGH);
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
void TaskUSB(void *pt)
{
  usbInit();
  while (true)
  {
    Usb.Task();
    vTaskDelay(5);
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

  *_to = toValue < -2047 ? -2047 : toValue;
}

// 读取xbox手柄输出值并更新到变量
void task_main_controller(void *pt)
{
  radio_data_t data;                              // 待发送
  TickType_t xLastWakeTime = xTaskGetTickCount(); // 最后唤醒时间
  const TickType_t xFrequency = pdMS_TO_TICKS(8); // 设置采样率 250hz

  while (true)
  {
    if (radio.status == RADIO_CONNECTED && Xbox.XboxOneConnected)
    {
      data.channel[0] = Xbox.getButtonPress(A);
      radio.set_data(&data);
    }
    xTaskDelayUntil(&xLastWakeTime, xFrequency);

    //   *TrigLT = Xbox.getButtonPress(L2);

    // if (Xbox.XboxOneConnected)
    // {
    //   // 字母键
    //   *BtnA = Xbox.getButtonPress(A);
    //   *BtnB = Xbox.getButtonPress(B);
    //   *BtnX = Xbox.getButtonPress(X);
    //   *BtnY = Xbox.getButtonPress(Y);

    //   // 功能键
    //   *BtnStart = Xbox.getButtonPress(START);
    //   // *BtnXbox = Xbox.getButtonPress(XBOX); // XXX 功能异常不使用
    //   *BtnSelect = Xbox.getButtonPress(BACK);
    //   *BtnShare = Xbox.getButtonPress(SYNC);

    //   // 肩键
    //   *BtnLB = Xbox.getButtonPress(L1);
    //   *BtnRB = Xbox.getButtonPress(R1);

    //   // 方向键
    //   *BtnDirUp = Xbox.getButtonPress(UP);
    //   *BtnDirDown = Xbox.getButtonPress(DOWN);
    //   *BtnDirLeft = Xbox.getButtonPress(LEFT);
    //   *BtnDirRight = Xbox.getButtonPress(RIGHT);

    //   // 摇杆
    //   syncAnalogHat(Xbox.getAnalogHat(LeftHatX), JoyLHori);
    //   syncAnalogHat(Xbox.getAnalogHat(LeftHatY), JoyLVert);
    //   *BtnLS = Xbox.getButtonPress(L3);

    //   syncAnalogHat(Xbox.getAnalogHat(RightHatX), JoyRHori);
    //   syncAnalogHat(Xbox.getAnalogHat(RightHatY), JoyRVert);
    //   *BtnRS = Xbox.getButtonPress(R3);

    //   // 扳机键
    //   *TrigLT = Xbox.getButtonPress(L2);
    //   *TrigRT = Xbox.getButtonPress(R2);
    // }

    xTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

// 设置变量引用指针
void Controller::setPointer()
{
  BtnA = &data.btnA;
  BtnB = &data.btnB;
  BtnX = &data.btnX;
  BtnY = &data.btnY;

  BtnShare = &data.btnShare;
  BtnStart = &data.btnStart;
  BtnSelect = &data.btnSelect;
  BtnXbox = &data.btnXbox;

  BtnLB = &data.btnLB;
  BtnRB = &data.btnRB;

  BtnLS = &data.btnLS;
  BtnRS = &data.btnRS;

  BtnDirUp = &data.btnDirUp;
  BtnDirLeft = &data.btnDirLeft;
  BtnDirRight = &data.btnDirRight;
  BtnDirDown = &data.btnDirDown;

  JoyLHori = &data.joyLHori;
  JoyLVert = &data.joyLVert;
  JoyRHori = &data.joyRHori;
  JoyRVert = &data.joyRVert;

  TrigLT = &data.trigLT;
  TrigRT = &data.trigRT;

  Connected = &Xbox.XboxOneConnected;
}

// 启动xbox控制器
void Controller::begin()
{
  // setPointer();
  xTaskCreate(TaskUSB, "taskUSB", 4096, NULL, 1, NULL);
  xTaskCreate(task_main_controller, "main_controller", 4096, NULL, 1, NULL);
}

bool Controller::getConnectStatus()
{
  return *Connected;
}