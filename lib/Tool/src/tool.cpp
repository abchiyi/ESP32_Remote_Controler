#include "tool.h"
#include "config.h"
#include "XBOX.h"
#include "array"

#define TAG "Tool"

uint8_t calculate_cksum(void *data, size_t len)
{
  uint8_t cksum = 0;
  uint8_t *p = (uint8_t *)data;
  for (size_t i = 0; i < len; i++)
  {
    cksum += p[i];
  }
  return cksum;
}

bool is_valid_mac(const uint8_t *mac, size_t len)
{
  // 检查输入参数
  if (mac == nullptr || len == 0)
  {
    return false;
  }

  // 检查每个字节
  for (size_t i = 0; i < len; i++)
  {
    // 如果字节为0x00或0xFF，则MAC地址无效
    if (mac[i] == 0x00 || mac[i] == 0xFF)
    {
      return false;
    }
  }

  // 所有字节都有效
  return true;
}

void get_setpoint_data_from_controller(packet_setpoint_t *pack)
{
  if (!Controller.is_connected() && pack == nullptr)
  {
    ESP_LOGW(TAG, "Controller is not connected, using default values");
    memset(pack, 0, sizeof(packet_setpoint_t));
    return;
  }

  // 将输入值从 -2048~2047 映射到 0~180°
  auto Range2Angle = [&](int value)
  {
    // TODO 该映射范围需要是可配置的
    return map(value, -2048, 2047, 0, 180);
  };

  auto GetThrust = [&](XBOX_INPUT_t analogHatKey, bool flip)
  {
    std::array<uint8_t, 2> trigger = {trigLT, trigRT};
    auto it = std::find(trigger.begin(), trigger.end(), analogHatKey);
    int16_t raw_thrust = Controller.getInput(analogHatKey);

    if (it != trigger.end())
      if (flip)
        return static_cast<uint16_t>(map(raw_thrust, 0, 1023, UINT16_MAX, 0));
      else
        return static_cast<uint16_t>(map(raw_thrust, 0, 1023, 0, UINT16_MAX));
    else
    {
      raw_thrust = abs(raw_thrust);
      if (flip)
        return static_cast<uint16_t>(map(raw_thrust, 0, 2048, UINT16_MAX, 0));
      else
        return static_cast<uint16_t>(map(raw_thrust, 0, 2048, 0, UINT16_MAX));
    }
  };

  int raw_pitch = Controller.getInput(CONFIG.PITCH);
  pack->PITCH = Range2Angle(CONFIG.PITCH_FLIP ? -raw_pitch : raw_pitch);

  int raw_roll = Controller.getInput(CONFIG.ROLL);
  pack->ROLL = Range2Angle(CONFIG.ROLL_FLIP ? -raw_roll : raw_roll);

  int raw_yaw = Controller.getInput(CONFIG.YAW);
  pack->YAW = Range2Angle(CONFIG.YAW_FLIP ? -raw_yaw : raw_yaw);

  int r_t = Controller.getInput(CONFIG.THRUST);
  pack->THRUST = GetThrust(CONFIG.THRUST, CONFIG.THRUST_FLIP);

  int16_t r_reverse = Controller.getInput(CONFIG.Reverse);
  pack->REVERSE = GetThrust(CONFIG.Reverse, CONFIG.Reverse_FLIP);

  auto breaker_0 = CONFIG.breaker[0];
  auto breaker_1 = CONFIG.breaker[1];

  if (breaker_0)
  {
    bool b0 = Controller.getInput(breaker_0);
    b0 = CONFIG.breaker_FLIP[0] ? !b0 : b0;
    if (breaker_1)
    {
      bool b1 = Controller.getInput(breaker_1);
      b1 = CONFIG.breaker_FLIP[1] ? !b1 : b1;
      pack->breaker = b0 && b1;
    }
    else
      pack->breaker = b0;
  }
}

#include <map>
