#include <esp_now.h>

typedef esp_err_t (*send_cb_t)(uint8_t *);

typedef enum radio_status
{
  RADIO_BEFORE_WAIT_CONNECTION,
  RADIO_WAIT_CONNECTION,

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
  radio_status_t status;
};

extern Radio radio;