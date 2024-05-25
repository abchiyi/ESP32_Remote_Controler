#include "storage_config.h"
#include "EEPROM.h"

#define TAG "STORAGE_CONFIG"

#define EEPROM_SIZE 512 // ESP32 EEPROM 容量

void storage_config::begin()
{
  EEPROM.begin(EEPROM_SIZE);
  uint32_t aa = 70;
  Serial.println();
  auto cc = this->create_sconfig(aa);
  ESP_LOGI(TAG, "aa %d", cc.ref);
  this->write(cc);
  aa = 0;
  ESP_LOGI(TAG, "Test eeprom %d", aa);
  this->read(cc);
  ESP_LOGI(TAG, "Test eeprom %d", aa);
};

storage_config Storage_config;

template <typename T>
sCongfig<T> storage_config::create_sconfig(T &value)
{
  sCongfig<T> config(value);
  config.addr_start = addr_start; // 储存起始地址

  ESP_LOGI(TAG, "Value %d, size %d addr_start %d",
           config.ref, config.size, config.addr_start);

  // TODO 当起始地址超过最大容量时，不在设置新的配置
  addr_start += config.size; // 更新储存起始地址
  return config;
}

template <typename T>
void storage_config::read(sCongfig<T> &config)
{
  EEPROM.get(config.addr_start, config.ref);
}

template <typename T>
void storage_config::write(sCongfig<T> &config)
{
  EEPROM.put(config.addr_start, config.ref);
}

void storage_config::save_all()
{
  EEPROM.commit();
  // TODO 保存时检查与EEPROM 中的值是否一致，一致则重新写入到储存中。
}