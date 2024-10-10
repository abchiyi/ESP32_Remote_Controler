/**
 * 控制器输入输出
 */

#define TAG "HID"
#include "controller.h"
#include "WouoUI.h"
#include "tool.h"

struct listener
{
  uint8_t send_count = 0; // 事件发送标记
  uint press_count = 0;   // 按下计时
  key_id event_id;        // 触发事件标识
  key_id event_id_sub;    // 触发事件标识

  virtual void update() = 0;
  virtual void sendEvent() = 0;
};

struct listenerButton : public listener
{
  ButtonEnum key;

  listenerButton(ButtonEnum key, key_id event_id)
  {
    this->event_id = event_id;
    this->key = key;
  };

  void sendEvent() override
  {
    WOUO_UI.dispatch_event(Event(event_id));
    send_count++;
  };

  void update() override
  {
    // XXX 该实现需要修正
    int btn_LPT = 125;
    int btn_SPT = 10;
    // btn_LPT = CONFIG_UI[BTN_LPT];
    // btn_SPT = CONFIG_UI[BTN_SPT];

    auto btn_Status = Xbox.getButtonPress(key);

    if (!btn_Status)
      send_count = 0; // 重置发送次数标记

    // 渐进加速，发送频率逐渐增加
    int hold_time = (btn_LPT - btn_LPT * send_count * 0.3);
    hold_time = hold_time < btn_SPT ? btn_SPT : hold_time;

    // 计算按钮按压时间
    press_count = 0;
    while (Xbox.getButtonPress(key))
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
    {
      ESP_LOGI(TAG, "send1");
      WOUO_UI.dispatch_event(Event(event_id));
    }
    else
    {
      ESP_LOGI(TAG, "send2");
      WOUO_UI.dispatch_event(Event(event_id_sub));
    }
  };

  void update() override
  {
    // XXX 该实现需要修正
    int btn_LPT = 100;
    int btn_SPT = 15;
    // btn_LPT = CONFIG_UI[BTN_LPT];
    // btn_SPT = CONFIG_UI[BTN_SPT];
    auto analogHatStatus = analogHatFilter(Xbox.getAnalogHat(key));
    int hold_time = btn_LPT;
    hold_time = btn_LPT * (1.0f - (abs((float)analogHatStatus) / 2048.0f));
    hold_time = hold_time < btn_SPT ? btn_SPT : hold_time;
    // 计算按钮按压时间
    press_count = 0;
    while (abs(analogHatStatus) > 100 && press_count <= hold_time)
    {
      press_count++;
      joy_is_up = analogHatStatus > 0;
      analogHatStatus = analogHatFilter(Xbox.getAnalogHat(key));
      vTaskDelay(1);
    }
    // 根据按压时间决定发出事件
    if (press_count >= btn_SPT)
    {
      ESP_LOGI(TAG, "AnalogHat %d, hold time %d", analogHatStatus, hold_time);

      this->sendEvent();
    }
  }
};

std::vector<listener *> listeners;

void task_hid(void *pt)
{
  TickType_t xLastWakeTime = xTaskGetTickCount(); // 最后唤醒时间
  const TickType_t xFrequency = pdMS_TO_TICKS(8); // 设置采样率 250hz
  while (true)
  {
    for (auto &listener : listeners)
      listener->update();
    // xTaskDelayUntil(&xLastWakeTime, xFrequency);
    vTaskDelay(8);
  }
};

void hid_begin()
{
  listeners.push_back(new listenerButton(UP, KEY_UP));
  listeners.push_back(new listenerButton(DOWN, KEY_DOWN));
  listeners.push_back(new listenerButton(START, KEY_MENU));
  listeners.push_back(new listenerJoystick(LeftHatY, KEY_UP, KEY_DOWN));
  xTaskCreate(task_hid, "task_hid", 1024 * 3, NULL, 1, NULL);
}