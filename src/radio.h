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

// 常规数据处理结构
struct Data
{
  int len;
  bool newData;
  uint8_t *mac;
  uint8_t *incomingData;
  Data() : newData(false){};

  uint8_t *get()
  {
    newData = false;
    return incomingData;
  }
};

// AP 扫描信息
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

// 握手数据结构
struct HANDSHAKE_DATA
{
  uint32_t code;
  uint8_t mac[ESP_NOW_ETH_ALEN];

  HANDSHAKE_DATA()
  {
    srand(time(0));
    code = rand();
  }
};

/**
 * @brief 无线通讯
 */
class Radio
{
private:
  void radioInit(); // 初始化无线

public:
  uint8_t *dataToSent;        // 待发送数据
  Data RecvData;              // 接收到的数据
  esp_now_peer_info *vehcile; // 无线控制器的配对信息
  radio_status_t status;      //  无线状态
  esp_err_t pairNewDevice();  // 配对新设备
  uint8_t timeOut = 50;       // 通讯超时, （timeOut * sendGap) ms
  uint8_t sendGap = 5;        // 发送间隔

  void begin(uint8_t *data_to_sent);
  void begin(uint8_t *data_to_sent, uint8_t send_timeout, uint8_t send_gap);
};

extern Radio radio;