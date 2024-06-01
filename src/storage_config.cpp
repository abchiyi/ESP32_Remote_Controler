#include "storage_config.h"
#include "EEPROM.h"
#include <algorithm> // 用于 std::equal
#include <iterator>  // 用于 std::begin, std::end
#include <tool.h>
#include <vector>

typedef void (*rw_cb)(storage_config &sc, bool mode); // 读写函数

std::vector<rw_cb> cb_func(0);

storage_config STORAGE_CONFIG;

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
  const uint16_t check_size = sizeof(config.check);
  config.addr_start = ADDR_START;                            // 数据地址
  config.addr_satrt_check = config.addr_start + config.size; // 检查码地址
  config.check = config.size + check_size;                   // 检查码内容
  ESP_LOGI(TAG, "Value pt %d, size %d addr_start %d",
           &config.ref, config.size, config.addr_start);

  // TODO 当起始地址超过最大容量时，不在设置新的配置
  ADDR_START += (config.size + check_size); // 更新储存起始地址
  return config;
}

template <typename T>
void read(sCongfig<T> &config)
{
  uint16_t check;
  EEPROM.get(config.addr_satrt_check, check);
  EEPROM.get(config.addr_start, config.ref);

  // 当校验码不一致时不从ROM读取，将保持配置不变
  if (check != config.check)
  {
    ESP_LOGI(TAG, "set in default config");
    ESP_LOGI(TAG, "check %d confgi.check %d, check start %d", check, config.check, config.addr_satrt_check);
    return;
  }
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
    EEPROM.put(config.addr_satrt_check, config.check);
    EEPROM.put(config.addr_start, config.ref);
  }
}

/** ----- configs ----- **/
auto config_ui = create_sconfig(CONFIG_UI);
auto config_radio = create_sconfig(CONFIG_RADIO.last_connected_device);

void cb1(storage_config &sc, bool mode)
{
  sc.RW(config_ui, mode);
};

// 自动保存任务
void TASK_AUTO_SAVE(void *pt)
{
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFreequency = 500;

  EEPROM.begin(EEPROM_SIZE);
  for (const auto &cb : cb_func)
    cb(STORAGE_CONFIG, 1);

  while (true)
  {
    for (const auto &cb : cb_func)
      cb(STORAGE_CONFIG, 0);
    vTaskDelay(5);
    EEPROM.commit();
    vTaskDelayUntil(&xLastWakeTime, xFreequency);
  }
}

void storage_config::begin()
{
  cb_func.push_back(cb1);

  xTaskCreate(TASK_AUTO_SAVE, "TASK_AUTO_SAVE", 1024 * 4, NULL, 1, NULL);
}

template <typename T>
void storage_config::RW(sCongfig<T> &config, bool mode)
{
  config.access([&]()
                { mode ? read(config) : write(config); }, portMAX_DELAY);
}