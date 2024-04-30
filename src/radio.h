#include <Arduino.h>
#include <esp_now.h>
#include <cstring>

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

// 接收到的数据
struct Data
{
  int len;
  bool newData;
  uint8_t *mac;
  uint8_t *incomingData;
  Data() : newData(false){};
};

struct AP_Info
{
  int8_t RSSI;
  String SSID;
  uint8_t CHANNEL;
  uint8_t MAC[ESP_NOW_ETH_ALEN];

  bool operator<(const AP_Info &obj) { return RSSI < obj.RSSI; };

  AP_Info(){};
  AP_Info(String ssid, int8_t rssi, int8_t channel, uint8_t *mac)
  {
    SSID = ssid;
    RSSI = rssi;
    CHANNEL = channel;
    memcpy(MAC, (const void *)mac, ESP_NOW_ETH_ALEN);
  };

  String toStr()
  {
    const char Template_[] = "SSID: %s, MAC:" MACSTR ", RSSI: %d, Channel: %d";
    char temp[std::strlen(Template_) + 30];
    sprintf(temp, Template_, SSID, MAC2STR(MAC), RSSI, CHANNEL);
    return String(temp);
  }
};

/**
 * @brief 被控设备无线通讯
 */
class Radio
{
private:
  void radioInit(); // 初始化无线
public:
  Data RecvData;              // 接收到的数据
  esp_now_peer_info *vehcile; // 无线控制器的配对信息
  void begin(send_cb_t cb, int send_gap_ms);
  radio_status_t status;     //  无线状态
  esp_err_t pairNewDevice(); // 配对新设备
};

extern Radio radio;