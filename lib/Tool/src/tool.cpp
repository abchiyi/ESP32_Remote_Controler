#include "tool.h"
#include "config.h"
#include "XBOX.h"

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

packet_setpoint_t get_setpoint_data_from_controller()
{
  static packet_setpoint_t setpoint_data;

  // 将输入值从 -2048~2047 映射到 0~180°
  auto Range2Angle = [&](int value)
  {
    // TODO 该映射范围需要是可配置的
    return map(value, -2048, 2047, 0, 180);
  };

  int raw_pitch = Controller.getAnalogHat(CONFIG.PITCH);
  setpoint_data.PITCH = Range2Angle(CONFIG.PITCH_FLIP ? -raw_pitch : raw_pitch);

  int raw_roll = Controller.getAnalogHat(CONFIG.ROLL);
  setpoint_data.ROLL = Range2Angle(CONFIG.ROLL_FLIP ? -raw_roll : raw_roll);

  int raw_yaw = Controller.getAnalogHat(CONFIG.YAW);
  setpoint_data.YAW = Range2Angle(CONFIG.YAW_FLIP ? -raw_yaw : raw_yaw);

  int r_t = Controller.getAnalogHat(CONFIG.THRUST);
  setpoint_data.THRUST = Range2Angle(CONFIG.THRUST_FLIP ? -r_t : r_t);

  bool reverse = Controller.getButtonPress(CONFIG.Reverse);
  setpoint_data.reverse = CONFIG.Reverse_FLIP ? !reverse : reverse;

  if (CONFIG.breaker[0])
  {
    bool break_1 = Controller.getButtonPress(CONFIG.breaker[0]);
    if (CONFIG.breaker[1])
    {
      bool break_2 = Controller.getButtonPress(CONFIG.breaker[1]);
      setpoint_data.breaker = CONFIG.breaker_FLIP[0]
                                  ? (!break_1 && !break_2)
                                  : (break_1 && break_2);
    }
    else
      setpoint_data.breaker = CONFIG.breaker_FLIP[0] ? !break_1 : break_1;
  }

  return setpoint_data;
}

#include <map>
