#include "storage_config.h"
#include "EEPROM.h"

// 被管理的配置
#include "radio.h"

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
  auto temp = config.ref;
  EEPROM.get(config.addr_start, temp);

  if (memcmp(&config.ref, &temp, config.size) != 0)
  {
    ESP_LOGI(TAG, "NEQ");
    EEPROM.put(config.addr_start, config.ref);
  }
  else
    ESP_LOGI(TAG, "EQ");
}

// configs
auto config_radio = create_sconfig(CONFIG_RADIO);

void save_all()
{
  EEPROM.begin(EEPROM_SIZE);
  write(config_radio);

  EEPROM.commit();
  EEPROM.end();
  // TODO 保存时检查与EEPROM 中的值是否一致，一致则重新写入到储存中。
}

void read_all()
{
  EEPROM.begin(EEPROM_SIZE);
  ESP_LOGI(TAG, "mac " MACSTR "", MAC2STR(config_radio.ref.last_connected_device));
  read(config_radio);
  ESP_LOGI(TAG, "mac " MACSTR "", MAC2STR(config_radio.ref.last_connected_device));
  EEPROM.end();
}
