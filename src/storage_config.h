#include <Arduino.h>
#include <functional>
#include <vector>
#include "EEPROM.h"
#include <tool.h>

typedef std::function<void(bool)> rw_cb; // 读写函数

template <typename T>
struct ConfigHandle
{
  uint16_t addr_start;       // 储存起始地址
  uint16_t check;            // 校验位，储存包含校验位在所rom占用的总大小
  uint16_t addr_satrt_check; // 校验位起始地址
  uint8_t size;              // 主数据大小
  T &ref;                    // 数据原始引用
  ConfigHandle(T &value) : ref(value)
  {
    size = sizeof(this->ref);
    xMutex = xSemaphoreCreateMutex();
    if (xMutex == NULL)
    {
      // TODO 处理错误: 互斥锁创建失败
    }
  }

  SemaphoreHandle_t xMutex;

  void access(std::function<void()> cb_fn, TickType_t block_time = 5)
  {
    if (xSemaphoreTake(xMutex, block_time) != pdTRUE)
      return;
    cb_fn();
    xSemaphoreGive(xMutex);
  }
};

template <typename T>
ConfigHandle<T> create_sconfig(T &value)
{
  static int16_t ADDR_START = 0;
  SemaphoreHandle_t xMutex;

  if (xMutex == NULL)
  {
    xMutex = xSemaphoreCreateMutex();
    if (xMutex == NULL)
    {
      // TODO 处理错误: 互斥锁创建失败
    }
  }

  ConfigHandle<T> config(value);
  const uint16_t check_size = sizeof(config.check);
  config.check = config.size + check_size; // 检查码内容
  if (xSemaphoreTake(xMutex, 100))
  {
    config.addr_start = ADDR_START;                            // 数据地址
    config.addr_satrt_check = config.addr_start + config.size; // 检查码地址

    // TODO 当起始地址超过最大容量时，不在设置新的配置
    ADDR_START += (config.size + check_size); // 更新储存起始地址
    xSemaphoreGive(xMutex);
    // cb_func.push_back([&](bool mode)
    //                   { STORAGE_CONFIG.RW(config, mode); });
    return config;
  }
  esp_system_abort("Config init time out");
}

class storage_config
{
private:
  std::vector<rw_cb> cb_func;

public:
  void begin(); // 启动储存服务

  template <typename T>
  void read(ConfigHandle<T> &config)
  {
    uint16_t check;
    EEPROM.get(config.addr_satrt_check, check);
    EEPROM.get(config.addr_start, config.ref);

    // 当校验码不一致时不从ROM读取，将保持配置不变
    if (check != config.check)
    {
      // ESP_LOGI(TAG, "set in default config");
      // ESP_LOGI(TAG, "check %d confgi.check %d, check start %d", check, config.check, config.addr_satrt_check);
      return;
    }
    EEPROM.get(config.addr_start, config.ref);
  }

  template <typename T>
  void write(ConfigHandle<T> &config)
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
      EEPROM.put(config.addr_satrt_check, config.check);
      EEPROM.put(config.addr_start, config.ref);
    }
  }

  void read_all()
  {
    for (const auto &cb : cb_func)
      cb(true);
  }
  void write_all()
  {
    for (const auto &cb : cb_func)
      cb(false);
  }

  void add(rw_cb cb)
  {
    cb_func.push_back(cb);
  }

  // 读写配置 true read, false write
  template <typename T>
  void RW(ConfigHandle<T> &config, bool mode)
  {
    config.access([&]()
                  { mode ? read(config) : write(config); }, portMAX_DELAY);
  };
};

extern storage_config STORAGE_CONFIG;
