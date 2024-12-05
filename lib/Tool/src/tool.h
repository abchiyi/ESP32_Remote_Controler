#include <cstddef>       // 用于 size_t
#include <algorithm>     // 用于 std::equal
#include "functional"    // std:func
#include "Preferences.h" // 储存

template <typename T, std::size_t N>
bool areArraysEqual(const T (&a)[N], const T (&b)[N])
{
  return std::equal(a, a + N, b);
};

/**
 * @brief 简化运行 Preferences 上下文
 * @param name_space 命名空间
 * @param cb_fn 执行回调
 */
void nvs_call(const char *name_space, std::function<void(Preferences &)> cb_fn);

typedef struct
{
  float VBUS_V;
  float POWER_W;
  float CURREN_A;
} power_info_t;

// 将一个32位浮点数拆分成两个16位的无符号整数
void splitFloat(float a, uint16_t *pa1, uint16_t *pa2);

// 将两个16位的无符号整数组合成原始的32位浮点数
float combineFloat(uint16_t pa1, uint16_t pa2);

// 过滤摇杆输出到-2048~2047;
int16_t analogHatFilter(int16_t value);
