#include <esp_now.h>

typedef esp_err_t (*send_cb_t)(uint8_t *);

struct recvData
{
  float volts;
};

/**
 * @brief 被控设备无线通讯
 */
class Radio
{
private:
public:
  static bool isPaired;       // 配对状态
  static recvData RecvData;   // 接收到的数据
  esp_now_peer_info *vehcile; // 无线控制器的配对信息
  void begin(send_cb_t cb, int send_gap_ms);
  bool getPairStatus();
};
