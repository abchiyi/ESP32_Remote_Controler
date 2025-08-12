#ifndef _TOOL_H_
#define _TOOL_H_

#include <cstddef>       // 用于 size_t
#include <algorithm>     // 用于 std::equal
#include "functional"    // std:func
#include "Preferences.h" // 储存
#include "crtp.h"        // CRTP 数据包定义

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

/**
 * @brief 检查MAC地址是否有效。
 *
 * 该函数检查MAC地址的每个字节是否为有效值。
 * 根据要求，MAC地址的每个字节不为0x00或0xFF时被认为是有效的。
 *
 * @param mac 指向MAC地址的指针。
 * @param len MAC地址的长度，通常为6字节。
 * @return 如果MAC地址有效返回true，否则返回false。
 */
bool is_valid_mac(const uint8_t *mac, size_t len);

/**
 * @brief 从控制器获取设定点数据。
 * @return 返回包含设定点数据的结构体 packet_setpoint_t
 */
packet_setpoint_t get_setpoint_data_from_controller();

#endif
