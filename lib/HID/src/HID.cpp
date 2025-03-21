/**
 * 控制器输入输出
 */
#define TAG "HID"
#include "controller.h"
#include "WouoUI.h"
#include "tool.h"
#include "config.h"
#include "radio.h"

struct listener
{
  uint8_t send_count = 0; // 事件发送标记
  uint press_count = 0;   // 按下计时
  key_id event_id;        // 触发事件标识
  key_id event_id_sub;    // 触发事件标识

  virtual void update() = 0;

  virtual void sendEvent()
  {
    WOUO_UI.dispatch_event(Event(event_id));
    WOUO_UI.dispatch_event(Event(KEY_WAKE));
    send_count++;
  };
};

struct listenerButton : public listener
{
  ButtonEnum key;

  listenerButton(ButtonEnum key, key_id event_id)
  {
    this->event_id = event_id;
    this->key = key;
  };

  void update() override
  {
    // XXX 该实现需要修正
    int btn_LPT = 125;
    int btn_SPT = 10;
    // btn_LPT = CONFIG_UI[BTN_LPT];
    // btn_SPT = CONFIG_UI[BTN_SPT];

    auto btn_Status = 0;
    // auto btn_Status = Xbox.getButtonPress(key);

    if (!btn_Status)
      send_count = 0; // 重置发送次数标记

    // 渐进加速，发送频率逐渐增加
    int hold_time = (btn_LPT - btn_LPT * send_count * 0.3);
    hold_time = hold_time < btn_SPT ? btn_SPT : hold_time;

    // 计算按钮按压时间
    press_count = 0;
    while (0)
    {
      if (press_count >= hold_time)
        break;
      press_count++;
      vTaskDelay(1);
    }
    // 根据按压时间决定发出事件
    if (press_count >= btn_SPT)
      this->sendEvent();
  }
};

struct listenerJoystick : public listener
{
  AnalogHatEnum key;
  bool joy_is_up;

  listenerJoystick(AnalogHatEnum key, key_id joy_UP, key_id joy_DOWN)
  {
    this->key = key;
    event_id = joy_UP;
    event_id_sub = joy_DOWN;
  };

  void sendEvent() override
  {
    if (joy_is_up)
      WOUO_UI.dispatch_event(Event(event_id));
    else
      WOUO_UI.dispatch_event(Event(event_id_sub));
  };

  void update() override
  {
    // XXX 该实现需要修正
    int btn_LPT = 100;
    int btn_SPT = 15;
    btn_LPT = CONFIG_UI[BTN_LPT];
    btn_SPT = CONFIG_UI[BTN_SPT];
    auto analogHatStatus = analogHatFilter(0);
    // auto analogHatStatus = analogHatFilter(Xbox.getAnalogHat(key));
    int hold_time = btn_LPT;
    hold_time = btn_LPT * (1.0f - (abs((float)analogHatStatus) / 2048.0f));
    hold_time = hold_time < btn_SPT ? btn_SPT : hold_time;
    // 计算按钮按压时间
    press_count = 0;
    while (abs(analogHatStatus) > 100 && press_count <= hold_time)
    {
      press_count++;
      joy_is_up = analogHatStatus > 0;
      analogHatStatus = analogHatFilter(0);
      // analogHatStatus = analogHatFilter(Xbox.getAnalogHat(key));
      vTaskDelay(1);
    }
    // 根据按压时间决定发出事件
    if (press_count >= btn_SPT)
      this->sendEvent();
  }
};

// 绑定按键到GUI事件
std::vector<std::tuple<ButtonEnum, key_id>> button = {
    {START, KEY_MENU},
    {DOWN, KEY_DOWN},
    {A, KEY_CONFIRM},
    {B, KEY_BACK},
    {UP, KEY_UP},
};

// 绑定摇杆到GUI事件
std::vector<std::tuple<AnalogHatEnum, key_id, key_id>> joystick = {
    {LeftHatY, KEY_UP, KEY_DOWN}};

// 监听器队列
std::vector<listener *> listeners;

void hid_begin()
{

  // 注册监听器到队列
  for (const auto &[button, event_id] : button)
    listeners.push_back(new listenerButton(button, event_id));

  for (const auto &[joystick, event_id, event_id_sub] : joystick)
    listeners.push_back(new listenerJoystick(joystick, event_id, event_id_sub));

  // 该任务通过扫描监听器队列，更新GUI事件
  auto task_hid = [](void *pt)
  {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(HZ2TICKS(150));
    while (true)
    {
      for (auto &listener : listeners)
        listener->update();
      xTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
  };

  xTaskCreatePinnedToCore(task_hid, "task_hid", 1024 * 3, NULL, TP_HIGHEST, NULL, 1);

  auto task_send_control_data = [](void *pt)
  {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(8);
    static radio_packet_t rp;
    auto crtp = (CRTPPacket *)rp.data;

    static float ROLL;
    static float PITCH;
    static float YAW;
    static uint16_t THRUST;

    auto match_angl = [](uint8_t angl, int16_t joy)
    {
      auto step = angl / 2048.f;
      return joy * step;
    };

    while (true)
    {

      ROLL = match_angl(40, Controller.joyLHori);
      PITCH = match_angl(30, Controller.joyRVert) * -1;
      YAW = match_angl(30, Controller.joyRHori);
      THRUST = Controller.joyLVert * 32;
      crtp->port = CRTP_PORT_SETPOINT;
      crtp->channel = 0;

      memcpy(&crtp->data[0], &ROLL, sizeof(ROLL));
      memcpy(&crtp->data[4], &PITCH, sizeof(PITCH));
      memcpy(&crtp->data[8], &YAW, sizeof(YAW));
      memcpy(&crtp->data[12], &THRUST, sizeof(THRUST));

      radio_send_packet(&rp);

      xTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
  };

  xTaskCreate(task_send_control_data, "task_send_control_data", 1024 * 3, NULL, TP_HIGHEST, NULL);
}
