#include "storage_config.h"
#include "EEPROM.h"
#include <algorithm> // 用于 std::equal
#include <iterator>  // 用于 std::begin, std::end
#include <tool.h>

/** 获取配置 **/
#include "radio.h"
#include "WouoUI.h"

#define TAG "STORAGE_CONFIG"

#define EEPROM_SIZE 512 // ESP32 EEPROM 容量

uint16_t ADDR_START = 0; // 起始地址

template <typename T>
sCongfig<T> create_sconfig(T &value)
{
  sCongfig<T> config(value);
  config.addr_start = ADDR_START; // 储存起始地址

  ESP_LOGI(TAG, "Value %d, size %d addr_start %d",
           config.ref, config.size, config.addr_start);

  // TODO 当起始地址超过最大容量时，不在设置新的配置
  ADDR_START += config.size; // 更新储存起始地址
  return config;
}

template <typename T>
void read(sCongfig<T> &config)
{
  EEPROM.get(config.addr_start, config.ref);
}

template <typename T>
void write(sCongfig<T> &config)
{
  T temp;
  memset(temp, 0, config.size);
  EEPROM.get(config.addr_start, temp);

  // 安全检查，避免对比出错
  static_assert(std::is_trivially_copyable<T>::value, "T must be trivially copyable");
  static_assert(std::is_standard_layout<T>::value, "T must have standard layout");

  bool write_data = std::is_array_v<T>
                        ? !areArraysEqual(temp, config.ref)
                        : !memcmp(&temp, &config.ref, config.size) != 0;

  if (write_data)
  {
    ESP_LOGI(TAG, "write config - size of %d", config.size);
    EEPROM.put(config.addr_start, config.ref);
  }
}

// configs
auto config_radio = create_sconfig(CONFIG_RADIO.last_connected_device);
auto config_ui = create_sconfig(CONFIG_UI);

void save_all()
{
  EEPROM.begin(EEPROM_SIZE);
  write(config_radio);
  write(config_ui);
  EEPROM.commit();
  EEPROM.end();
  // TODO 保存时检查与EEPROM 中的值是否一致，一致则重新写入到储存中。
}

void read_all()
{
  EEPROM.begin(EEPROM_SIZE);
  read(config_radio);
  read(config_ui);
  EEPROM.end();
}
