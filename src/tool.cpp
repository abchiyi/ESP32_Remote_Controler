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

#include "XBOXONE.h"
// 0 ~ 2047
#define DEAD_ZONE 8000.0F
const int maxLength = 32768;
const int resolution = 2048;

const float_t step = (float_t)(maxLength - DEAD_ZONE) / (float_t)resolution;

int16_t analogHatFilter(int16_t value)
{
  // 计算除去死区后摇杆的值
  const int16_t t_from = value < 0
                             ? value - -DEAD_ZONE
                             : value - DEAD_ZONE;

  int16_t toValue = value < (DEAD_ZONE * -1) || value > DEAD_ZONE
                        ? (float_t)t_from / step
                        : 0;

  return toValue < -2048 ? -2048 : toValue;
}