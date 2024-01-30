#include <controller.h>
#include <XBOXONE.h>
#include <esp_log.h>
#include <Usb.h>

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
    vTaskDelay(1);
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
void TaskUpdate(void *pt)
{
  while (true)
  {

    // if (Xbox.XboxOneConnected)
    // {
    //   if (Xbox.getAnalogHat(LeftHatX) > 7500 || Xbox.getAnalogHat(LeftHatX) < -7500 || Xbox.getAnalogHat(LeftHatY) > 7500 || Xbox.getAnalogHat(LeftHatY) < -7500 || Xbox.getAnalogHat(RightHatX) > 7500 || Xbox.getAnalogHat(RightHatX) < -7500 || Xbox.getAnalogHat(RightHatY) > 7500 || Xbox.getAnalogHat(RightHatY) < -7500)
    //   {
    //     if (Xbox.getAnalogHat(LeftHatX) > 7500 || Xbox.getAnalogHat(LeftHatX) < -7500)
    //     {
    //       Serial.print(F("LeftHatX: "));
    //       Serial.print(Xbox.getAnalogHat(LeftHatX));
    //       Serial.print("\t");
    //     }
    //     if (Xbox.getAnalogHat(LeftHatY) > 7500 || Xbox.getAnalogHat(LeftHatY) < -7500)
    //     {
    //       Serial.print(F("LeftHatY: "));
    //       Serial.print(Xbox.getAnalogHat(LeftHatY));
    //       Serial.print("\t");
    //     }
    //     if (Xbox.getAnalogHat(RightHatX) > 7500 || Xbox.getAnalogHat(RightHatX) < -7500)
    //     {
    //       Serial.print(F("RightHatX: "));
    //       Serial.print(Xbox.getAnalogHat(RightHatX));
    //       Serial.print("\t");
    //     }
    //     if (Xbox.getAnalogHat(RightHatY) > 7500 || Xbox.getAnalogHat(RightHatY) < -7500)
    //     {
    //       Serial.print(F("RightHatY: "));
    //       Serial.print(Xbox.getAnalogHat(RightHatY));
    //     }
    //     Serial.println();
    //   }

    //   if (Xbox.getButtonPress(L2) > 0 || Xbox.getButtonPress(R2) > 0)
    //   {
    //     if (Xbox.getButtonPress(L2) > 0)
    //     {
    //       Serial.print(F("L2: "));
    //       Serial.print(Xbox.getButtonPress(L2));
    //       Serial.print("\t");
    //     }
    //     if (Xbox.getButtonPress(R2) > 0)
    //     {
    //       Serial.print(F("R2: "));
    //       Serial.print(Xbox.getButtonPress(R2));
    //       Serial.print("\t");
    //     }
    //     Serial.println();
    //   }

    //   // Set rumble effect
    //   static uint16_t oldL2Value, oldR2Value;
    //   if (Xbox.getButtonPress(L2) != oldL2Value || Xbox.getButtonPress(R2) != oldR2Value)
    //   {
    //     oldL2Value = Xbox.getButtonPress(L2);
    //     oldR2Value = Xbox.getButtonPress(R2);
    //     uint8_t leftRumble = map(oldL2Value, 0, 1023, 0, 255); // Map the trigger values into a byte
    //     uint8_t rightRumble = map(oldR2Value, 0, 1023, 0, 255);
    //     if (leftRumble > 0 || rightRumble > 0)
    //       Xbox.setRumbleOn(leftRumble, rightRumble, leftRumble, rightRumble);
    //     else
    //       Xbox.setRumbleOff();
    //   }

    //   if (Xbox.getButtonClick(UP))
    //     Serial.println(F("Up"));
    //   if (Xbox.getButtonClick(DOWN))
    //     Serial.println(F("Down"));
    //   if (Xbox.getButtonClick(LEFT))
    //     Serial.println(F("Left"));
    //   if (Xbox.getButtonClick(RIGHT))
    //     Serial.println(F("Right"));

    //   if (Xbox.getButtonClick(START))
    //     Serial.println(F("Start"));
    //   if (Xbox.getButtonClick(BACK))
    //     Serial.println(F("Back"));
    //   if (Xbox.getButtonClick(XBOX))
    //     Serial.println(F("Xbox"));
    //   if (Xbox.getButtonClick(SYNC))
    //     Serial.println(F("Sync"));

    //   if (Xbox.getButtonClick(L1))
    //     Serial.println(F("L1"));
    //   if (Xbox.getButtonClick(R1))
    //     Serial.println(F("R1"));
    //   if (Xbox.getButtonClick(L2))
    //     Serial.println(F("L2"));
    //   if (Xbox.getButtonClick(R2))
    //     Serial.println(F("R2"));
    //   if (Xbox.getButtonClick(L3))
    //     Serial.println(F("L3"));
    //   if (Xbox.getButtonClick(R3))
    //     Serial.println(F("R3"));

    //   if (Xbox.getButtonClick(A))
    //     Serial.println(F("A"));
    //   if (Xbox.getButtonClick(B))
    //     Serial.println(F("B"));
    //   if (Xbox.getButtonClick(X))
    //     Serial.println(F("X"));
    //   if (Xbox.getButtonClick(Y))
    //     Serial.println(F("Y"));
    // }

    if (Xbox.XboxOneConnected)
    {
      // 字母键
      *BtnA = Xbox.getButtonPress(A);
      *BtnB = Xbox.getButtonPress(B);
      *BtnX = Xbox.getButtonPress(X);
      *BtnY = Xbox.getButtonPress(Y);

      // 功能键
      *BtnStart = Xbox.getButtonPress(START);
      // *BtnXbox = Xbox.getButtonPress(XBOX); // XXX 功能异常不使用
      *BtnSelect = Xbox.getButtonPress(BACK);
      *BtnShare = Xbox.getButtonPress(SYNC);

      // 肩键
      *BtnLB = Xbox.getButtonPress(L1);
      *BtnRB = Xbox.getButtonPress(R1);

      // 方向键
      *BtnDirUp = Xbox.getButtonPress(UP);
      *BtnDirDown = Xbox.getButtonPress(DOWN);
      *BtnDirLeft = Xbox.getButtonPress(LEFT);
      *BtnDirRight = Xbox.getButtonPress(RIGHT);

      // 摇杆
      syncAnalogHat(Xbox.getAnalogHat(LeftHatX), JoyLHori);
      syncAnalogHat(Xbox.getAnalogHat(LeftHatY), JoyLVert);
      *BtnLS = Xbox.getButtonPress(L3);

      syncAnalogHat(Xbox.getAnalogHat(RightHatX), JoyRHori);
      syncAnalogHat(Xbox.getAnalogHat(RightHatY), JoyRVert);
      *BtnRS = Xbox.getButtonPress(R3);

      // 扳机键
      *TrigLT = Xbox.getButtonPress(L2);
      *TrigRT = Xbox.getButtonPress(R2);
    }

    vTaskDelay(10);
  }
}

// 设置变量引用指针
void Controller::setPointer()
{
  BtnA = &btnA;
  BtnB = &btnB;
  BtnX = &btnX;
  BtnY = &btnY;

  BtnShare = &btnShare;
  BtnStart = &btnStart;
  BtnSelect = &btnSelect;
  BtnXbox = &btnXbox;

  BtnLB = &btnLB;
  BtnRB = &btnRB;

  BtnLS = &btnLS;
  BtnRS = &btnRS;

  BtnDirUp = &btnDirUp;
  BtnDirLeft = &btnDirLeft;
  BtnDirRight = &btnDirRight;
  BtnDirDown = &btnDirDown;

  JoyLHori = &joyLHori;
  JoyLVert = &joyLVert;
  JoyRHori = &joyRHori;
  JoyRVert = &joyRVert;

  TrigLT = &trigLT;
  TrigRT = &trigRT;

  Connected = &Xbox.XboxOneConnected;
}

// 启动xbox控制器
void Controller::begin()
{
  setPointer();
  delay(5);
  xTaskCreate(TaskUSB, "taskUSB", 4096, NULL, 1, NULL);
  xTaskCreate(TaskUpdate, "TaskUpdate", 4096, NULL, 1, NULL);
}

// ↓this code from XboxControllerNotificationParser lib
String Controller::toString()
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
  // clang-format on
  return str;
}

bool Controller::getConnectStatus()
{
  return *Connected;
}