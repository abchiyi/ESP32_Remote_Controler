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
  // AP_Info(String ssid, String BSSIDstr, int32_t channel, int32_t rssi)
  // {
  //   sscanf(BSSIDstr.c_str(), "%x:%x:%x:%x:%x:%x",
  //          &MAC[0], &MAC[1], &MAC[2], &MAC[3], &MAC[4], &MAC[5]);
  //   CHANNEL = channel;
  //   SSID = ssid;
  //   RSSI = rssi;
  // };
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