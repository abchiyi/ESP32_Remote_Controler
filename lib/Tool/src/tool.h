#ifndef _TOOL_H_
#define _TOOL_H_

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

/**
 * @brief 计算给定数据的校验和。
 * @param data 指向要计算校验和的数据的指针。
 * @param len 数据的长度，以字节为单位。
 * @return 返回计算得到的校验和，类型为 uint8_t。
 */
IRAM_ATTR uint8_t calculate_cksum(void *data, size_t len);

/**
 * @brief 将频率（Hz）转换为滴答数（Ticks）。
 *
 * 该函数用于将给定的频率值（以赫兹为单位）转换为对应的滴答数。
 * 在 ESP32 ARDUINO 中，1 Tick 等于 1 毫秒。
 *
 * @param hz 频率值，单位为赫兹（Hz）。
 * @return 返回转换后的滴答数（Ticks）。
 */
#define HZ2TICKS(hz) ((TickType_t)(1000.00f / hz))

#endif
