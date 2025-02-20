#include <Arduino.h>
#include <esp_now.h>
#include <cstring>
#include <vector>
#include <array>
#include <crtp.h>

// 通讯结构体
typedef struct
{
  union
  {
    struct
    {
      uint8_t data[sizeof(CRTPPacket::raw)];
      uint8_t checksum;
    };
    uint8_t raw[32];
  };
} __attribute__((packed)) radio_packet_t;

typedef struct
{
  std::function<esp_err_t(radio_packet_t *)> recv; // 接收回调
  std::function<esp_err_t(radio_packet_t *)> send; // 发送回调
  std::function<bool(void)> is_connected;          // 验证连接
  std::function<esp_err_t(void)> start;            // 启动通信链路
  std::function<esp_err_t(void)> rest;             // 重置通讯链路
} radio_link_operation_t;

/**
 * @brief 无线通讯
 */
class Radio
{
  friend void TaskRadioMainLoop(void *pt);

private:
public:
  int Frequency; // 主循环运行频率

  void begin();

  void update_loop_metrics()
  {
    static auto counter = 0;
    static TickType_t last_run;
    static auto buffer = 0;
    auto now = xTaskGetTickCount();
    buffer += (now - last_run);
    counter++;
    if (counter >= 100)
    {
      Frequency = 1000 / (buffer / 100);
      counter = 0;
      buffer = 0;
    }
    last_run = now;
  };
};

extern Radio RADIO;
