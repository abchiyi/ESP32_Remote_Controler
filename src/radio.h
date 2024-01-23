#include <esp_now.h>

/**
 * @brief 被控设备无线通讯
 */
class Radio
{
private:
  bool *isPaired;

public:
  esp_now_peer_info *vehcile; // 无线控制器的配对信息
  int channel = 1;            // XXX 需要可自定义的通道
  void begin(void *Presend_data, int send_gap_ms);
  bool getPairStatus();
};
