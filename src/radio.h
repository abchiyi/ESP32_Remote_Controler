#include <Arduino.h>
#include <esp_now.h>
#include <cstring>

#define MAX_CHANNEL 8 // 最大控制通道数量

typedef esp_err_t (*send_cb_t)(uint8_t *);
typedef uint8_t mac_addr_t[ESP_NOW_ETH_ALEN];

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

// AP 扫描信息
typedef struct ap_info
{
  int8_t RSSI;
  String SSID;
  uint8_t CHANNEL;
  mac_addr_t MAC;

  bool operator<(const ap_info &obj) { return RSSI < obj.RSSI; };

  ap_info(){};
  ap_info(String ssid, int8_t rssi, int8_t channel, uint8_t *mac)
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
} ap_info_t;

// 通讯结构体
typedef struct radio_data
{
  mac_addr_t mac_addr;           // 发送者地址
  uint16_t channel[MAX_CHANNEL]; // 通道信息
} radio_data_t;

/**
 * @brief 无线通讯
 */
class Radio
{
private:
  void radioInit();      // 初始化无线
  mac_addr_t __mac_addr; // 此设备mac 地址

public:
  uint8_t *dataToSent;         // 待发送数据
  esp_now_peer_info peer_info; // 配对信息
  radio_status_t status;       // 无线状态
  esp_err_t pairNewDevice();   // 配对新设备
  uint8_t timeOut = 50;        // 通讯超时, （timeOut * sendGap) ms
  uint8_t sendGap = 5;         // 发送间隔

  void begin(uint8_t *data_to_sent);
  void begin(uint8_t *data_to_sent, uint8_t send_timeout, uint8_t send_gap);
};

extern Radio radio;