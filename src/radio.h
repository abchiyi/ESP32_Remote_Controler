#include <esp_now.h>

typedef esp_err_t (*send_cb_t)(uint8_t *);

/**
 * @brief 被控设备无线通讯
 */
class Radio
{
private:
public:
  static bool isPaired;
  esp_now_peer_info *vehcile; // 无线控制器的配对信息
  int channel = 1;            // XXX 需要可自定义的通道
  void begin(send_cb_t cb, int send_gap_ms);
  bool getPairStatus();
};
