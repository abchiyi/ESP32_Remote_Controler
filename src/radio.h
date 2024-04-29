#include <esp_now.h>
#include <Arduino.h>
#include <esp_err.h>

typedef esp_err_t (*send_cb_t)(uint8_t *);

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
} radio_status_t;

struct recvData
{
  float volts;
  int gear;
  int ang;
};

struct AP_Info
{
  int8_t RSSI;
  String SSID;
  uint8_t CHANNEL;
  uint8_t MAC[ESP_NOW_ETH_ALEN];

  bool operator<(const AP_Info &obj)
  {
    // a>b的时候才返回true, 期待a是较大的元素。
    // 把较大的元素放在前面，降序排序
    return RSSI < obj.RSSI;
  }
  AP_Info(String ssid, int8_t rssi, int8_t channel, String mac)
  {
    SSID = ssid;
    RSSI = rssi;
    CHANNEL = channel;
    memcpy(MAC, mac.c_str(), ESP_NOW_ETH_ALEN);
  };
  AP_Info(){};
};

/**
 * @brief 被控设备无线通讯
 */
class Radio
{
private:
  void radioInit(); // 初始化无线
public:
  static bool isPaired;       // 配对状态
  static recvData RecvData;   // 接收到的数据
  esp_now_peer_info *vehcile; // 无线控制器的配对信息
  void begin(send_cb_t cb, int send_gap_ms);
  bool getPairStatus();
  radio_status_t status;     //  无线状态
  esp_err_t pairNewDevice(); // 配对新设备
};

extern Radio radio;