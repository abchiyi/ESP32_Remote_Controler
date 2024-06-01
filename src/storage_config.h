#include <Arduino.h>
#include <vector>
#include <functional>

template <typename T>
struct sCongfig
{
  uint16_t addr_start;       // 储存起始地址
  uint16_t check;            // 校验位，储存包含校验位在所rom占用的总大小
  uint16_t addr_satrt_check; // 校验位起始地址
  uint8_t size;              // 主数据大小
  T &ref;                    // 数据原始引用
  sCongfig(T &value) : ref(value)
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

class storage_config
{
private:
public:
  void begin(); // 启动储存服务

  template <typename T>
  void RW(sCongfig<T> &config, bool mode); // 读写配置 true read, false write
};

extern storage_config STORAGE_CONFIG;