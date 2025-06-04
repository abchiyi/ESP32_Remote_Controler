#include "tool.h"

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
