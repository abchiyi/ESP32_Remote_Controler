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