#include <Arduino.h>
#include <esp_now.h>
#include <cstring>
#include <vector>
#include <array>

#define MAX_CHANNEL 8         // 最大控制通道数量
#define MAX_DEVICES_NUMBER 10 // 最大储存已配对设备

typedef std::array<unsigned char, ESP_NOW_ETH_ALEN> mac_t; // MAC 地址

typedef std::function<void()> radio_cb_fn_t;

typedef enum radio_status
{
  RADIO_BEFORE_PAIR_DEVICE,
  RADIO_PAIR_DEVICE,

  RADIO_BEFORE_CONNECTING,
  RADIO_CONNECTING,

  RADIO_BEFORE_CONNECTED,
  RADIO_CONNECTED,

  RADIO_BEFORE_DISCONNECT,
  RADIO_DISCONNECT,

  RADIO_NO_CONNECTION_BEFORE,
  RADIO_NO_CONNECTION,

  RADIO_IN_SCAN_BEFORE,
  RADIO_IN_SCAN,

} radio_status_t;

// AP 扫描信息
typedef struct ap_info
{
  int8_t RSSI;
  String SSID;
  uint8_t CHANNEL;
  mac_t MAC;

  bool operator<(const ap_info &obj) { return RSSI < obj.RSSI; };

  ap_info() {};
  ap_info(String ssid, int8_t rssi, int8_t channel, uint8_t *mac)
  {
    SSID = ssid;
    RSSI = rssi;
    CHANNEL = channel;
    memcpy(&MAC, (const void *)mac, ESP_NOW_ETH_ALEN);
  };

  String toStr()
  {
    const char Template_[] = "SSID: %s, MAC:" MACSTR ", RSSI: %d, Channel: %d";
    char temp[std::strlen(Template_) + 30];
    sprintf(temp, Template_, SSID, MAC2STR(MAC), RSSI, CHANNEL);
    return String(temp);
  }
} ap_info_t;

// 通讯结构体
typedef struct radio_data
{
  mac_t mac_addr;                // 发送者地址
  uint16_t channel[MAX_CHANNEL]; // 通道信息
} radio_data_t;

// 检查mac是否有效
bool macOK(const mac_t &arr);

/**
 * @brief 无线通讯
 */
class Radio
{
  friend void TaskRadioMainLoop(void *pt);

private:
  mac_t __mac_addr; // 此设备mac 地址

  std::array<mac_t, 6> paired_devices;

public:
  auto clear()
  {
    paired_devices.fill({
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
    });
  }

  mac_t last_connected_device; // 最后连接过的设备
  // 获取一个有效mac地址的序列
  auto get_copy()
  {
    std::vector<mac_t> temp;
    for (auto mac : paired_devices)
      if (macOK(mac))
        temp.push_back(mac);

    return temp;
  };

  // 将mac添加到末尾，并删除首位
  void push_back(mac_t mac)
  {
    for (size_t i = 0; i < paired_devices.size() - 1; i++)
      paired_devices[i] = paired_devices[i + 1];
    paired_devices[5] = mac;
  };

  void remove(mac_t _mac)
  {
    auto array = get_copy();
    paired_devices = {};
    for (auto mac : array)
      if (mac != _mac)
        push_back(mac);
  }

  void config_clear();
  void confgi_save();
  esp_err_t scan_ap();                     // 扫描AP
  esp_err_t pairNewDevice();               // 配对新设备
  esp_err_t connect_to(const ap_info_t *); // 连接到设备

  int8_t RSSI = 0;
  ap_info_t target_ap;         // 目标AP
  esp_now_peer_info peer_info; // 配对信息
  radio_status_t status;       // 无线状态

  template <typename T>
  bool send(const T &data);

  void begin();
  void begin(uint8_t send_timeout);

  // TODO 移除get & set 使用回调处理
  esp_err_t get_data(radio_data_t *data); // 读取收到的数据
  esp_err_t set_data(radio_data_t *data); // 设置要发送的数据

  /**
   * freeRTOS 在esp32 一个 tick 为 1ms
   * 以下定义的超时值单位为 1ms
   */
  uint8_t timeout_resend = 70;      // 超时重发
  uint8_t resend_count = 5;         // 超时重发次数
  uint8_t timeout_disconnect = 250; // 超时断开连接

  std::vector<ap_info_t> AP; // 发现的AP，scan_ap执行完毕或不处于扫描状态时读取

  int Frequency = 10; // 主循环运行频率
  int run_time = 10;  // 主循环单次执行时间

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

  radio_cb_fn_t conected_before_send; // 数据发送前回调

  // 回调数据 当 AP 扫描完成后
  radio_cb_fn_t adter_scan_ap_comp;
};

extern Radio RADIO;
