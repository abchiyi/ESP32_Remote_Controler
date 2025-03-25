#include <tool.h>

void nvs_call(const char *name_space, std::function<void(Preferences &)> cb_fn)
{
  Preferences PREFS;
  PREFS.begin(name_space);
  cb_fn(PREFS);
  PREFS.end();
}

// 将一个32位浮点数拆分成两个16位的无符号整数
void splitFloat(float a, uint16_t *pa1, uint16_t *pa2);

// 将两个16位的无符号整数组合成原始的32位浮点数
float combineFloat(uint16_t pa1, uint16_t pa2);

// 将一个32位浮点数拆分成两个16位的无符号整数
void splitFloat(float a, uint16_t *pa1, uint16_t *pa2)
{
  uint32_t *p = (uint32_t *)(&a);
  *pa1 = (*p) >> 16;    // 取高16位
  *pa2 = (*p) & 0xFFFF; // 取低16位
}

// 将两个16位的无符号整数组合成原始的32位浮点数
float combineFloat(uint16_t pa1, uint16_t pa2)
{
  uint32_t p = ((uint32_t)pa1 << 16) | (uint32_t)pa2;
  return *((float *)(&p));
}

short analogHatFilter(uint16_t rawValue)
{
  const int mid = 32768;      // 摇杆中间原始值
  const int deadzone = 4000;  // 虚位死区阈值（根据实际硬件调整）
  const int maxOutput = 2048; // 目标最大输出值

  // 计算相对于中间值的偏移量（范围：-32768 ~ +32767）
  int offset = (int)rawValue - mid;

  // 死区过滤：中间虚位部分直接返回0
  if (abs(offset) <= deadzone)
    return 0;

  // 确定方向（正：右摇杆，负：左摇杆）
  int direction = (offset > 0) ? 1 : -1;

  // 计算有效偏移量（扣除死区后的实际偏移）
  int effectiveOffset = abs(offset) - deadzone;

  // 计算有效范围的最大偏移量（左/右可能不对称）
  int maxEffectiveOffset;
  if (direction == 1)
  {
    // 右侧有效范围：从 (mid + deadzone) 到 65535
    maxEffectiveOffset = 65535 - (mid + deadzone);
  }
  else
  {
    // 左侧有效范围：从 0 到 (mid - deadzone)
    maxEffectiveOffset = mid - deadzone;
  }

  // 线性缩放至目标范围 [-2048, 2048]
  int scaledValue = (effectiveOffset * maxOutput) / maxEffectiveOffset;

  // 确保不超出目标范围
  scaledValue = (scaledValue > maxOutput) ? maxOutput : scaledValue;

  // 返回带方向的最终值
  return (short)(direction * scaledValue);
}

IRAM_ATTR uint8_t calculate_cksum(void *data, size_t len)
{
  auto c = (unsigned char *)data;
  unsigned char cksum = 0;
  for (int i = 0; i < len; i++)
    cksum += *(c++);
  return cksum;
}
