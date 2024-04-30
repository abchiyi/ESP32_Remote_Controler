#include <radio.h>
#include <WiFi.h>
#include <vector>
#include <esp_wifi.h>
#include <esp_mac.h>
#include <algorithm>

#define TAG "Radio"
#define SLAVE_KE_NAME "Slave"

using namespace std;

esp_now_peer_info_t slave;

int CONNECT_TIMEOUT = 500;              // ms // 连接同步等待时间
int ConnectedTimeOut = CONNECT_TIMEOUT; // ms
const int MinSendGapMs = 8;
recvData Radio::RecvData;
bool PairRuning = false;
int Send_gap_ms = 0;
Radio radio;

send_cb_t SENDCB;

bool Pair(esp_now_peer_info_t slave)
{
  ESP_LOGI(TAG, "Pair to " MACSTR "", MAC2STR(slave.peer_addr));
  esp_err_t addStatus = esp_now_add_peer(&slave);

  switch (addStatus)
  {
  case ESP_OK:
    ESP_LOGI(TAG, "Pair success");
    return true;

  case ESP_ERR_ESPNOW_EXIST:
    ESP_LOGE(TAG, "Peer Exists");
    return true;

  case ESP_ERR_ESPNOW_NOT_INIT:
    ESP_LOGE(TAG, "ESPNOW Not Init");
    return false;

  case ESP_ERR_ESPNOW_ARG:
    ESP_LOGE(TAG, "Invalid Argument");
    return false;

  case ESP_ERR_ESPNOW_FULL:
    ESP_LOGE(TAG, "Peer list full");
    return false;

  case ESP_ERR_ESPNOW_NO_MEM:
    ESP_LOGE(TAG, "Out of memory");
    return false;

  default:
    ESP_LOGE(TAG, "Not sure what's going on");
    return false;
  }
}

/**
 * @brief 配对新设备
 * @return esp_err_t ESP_OK/ESP_FAIL
 */
esp_err_t Radio::pairNewDevice()
{
  vector<AP_Info> ap_info(0);
  int16_t scanResults = 0;

  ESP_LOGI(TAG, "Start scan");
  // 扫描1~13信道，过滤并储存所有查找到的AP信息
  for (size_t channel_i = 1; channel_i < 14; channel_i++)
  {
    scanResults = WiFi.scanNetworks(0, 0, 0, 100, channel_i);
    if (scanResults == 0) // 当前信道没有扫描到AP跳转到下一信道扫描
      continue;
    for (int i = 0; i < scanResults; i++)
      // 过滤 AP,由特定字符起始则被视为一个可以配对的设备
      if (WiFi.SSID(i).indexOf(SLAVE_KE_NAME) == 0)
        ap_info.emplace_back(AP_Info(
            WiFi.SSID(i),
            WiFi.RSSI(i),
            WiFi.channel(i),
            WiFi.BSSID(i)));
    vTaskDelay(1);
    WiFi.scanDelete(); // 清除扫描信息
  }

  // 遍历所有储存的AP
  for (size_t i = 0; i < ap_info.size(); i++)
    ESP_LOGI(TAG, "SSID: %s, MAC:" MACSTR ", RSSI: %d, Channel: %d",
             ap_info[i].SSID,
             MAC2STR(ap_info[i].MAC),
             ap_info[i].RSSI,
             ap_info[i].CHANNEL);
  ESP_LOGI(TAG, "AP scan comp");

  if (ap_info.size() < 1) // 没有AP被扫描到
    return ESP_FAIL;

  // 查找信号最强的AP
  int8_t targetIndex =
      max_element(ap_info.begin(), ap_info.end()) - ap_info.begin();

  ESP_LOGI(TAG,
           "Target -- SSID: %s, MAC:" MACSTR ", RSSI: %d, Channel: %d", ap_info[targetIndex].SSID,
           MAC2STR(ap_info[targetIndex].MAC),
           ap_info[targetIndex].RSSI,
           ap_info[targetIndex].CHANNEL);

  memcpy(slave.peer_addr, ap_info[targetIndex].MAC, ESP_NOW_ETH_ALEN);
  slave.channel = ap_info[targetIndex].CHANNEL;
  slave.ifidx = WIFI_IF_STA;
  slave.encrypt = false;
  Pair(slave);

  const char data[6] = "Hello";
  esp_now_send(slave.peer_addr, (uint8_t *)&data, sizeof(data));

  ESP_LOGI(TAG, "COMP");
  return ESP_OK;
}

void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  ConnectedTimeOut = CONNECT_TIMEOUT; // 接收到数据时重置超时
  memcpy(&Radio::RecvData, incomingData, sizeof(Radio::RecvData));
  ESP_LOGI(TAG, "RECV from " MACSTR ".", MAC2STR(mac));

  // if (PairRuning)
  // {
  //   ConnectedTimeOut = CONNECT_TIMEOUT; // 接收到数据时重置超时
  //   // slave.channel = CHANNEL;
  //   slave.encrypt = 0;
  //   memcpy(slave.peer_addr, mac, 6);

  //   if (Pair(slave))
  //   {
  //     PairRuning = false;
  //     Radio::isPaired = true;
  //   }
  // }
}

void Radio::radioInit()
{
  // set wifi
  ESP_LOGI(TAG, "Init wifi");
  WiFi.enableLongRange(false);

  // 启动WIFI IN STA mode
  if (WiFi.mode(WIFI_STA))
    ESP_LOGI(TAG, "WIFI Start in STA,MAC: %s, CHANNEL: %u",
             WiFi.macAddress().c_str(), WiFi.channel());
  else
    esp_system_abort("WIFI Start FAIL");

  // 设置 ESPNOW 通讯速率
  esp_wifi_config_espnow_rate(WIFI_IF_STA, WIFI_PHY_RATE_LORA_500K) == ESP_OK
      ? ESP_LOGI(TAG, "Set ESPNOW WIFI_PHY_RATE_LORA_500K")
      : ESP_LOGI(TAG, "Set ESPNOW RATE FAIL");

  // 设置最大 TX Power 到 20db
  esp_wifi_set_max_tx_power(84) == ESP_OK
      ? ESP_LOGI(TAG, "Set TxPower 20db")
      : ESP_LOGI(TAG, "Set TxPower Fail");

  // 设置 ESPNOW
  if (esp_now_init() != ESP_OK)
    esp_system_abort("ESP NOW Init Fail.");
  else
    ESP_LOGI(TAG, "ESP NOW init success");

  // 注册接收回调
  esp_now_register_recv_cb(onDataRecv) == ESP_OK
      ? ESP_LOGI(TAG, "Register recv cb success")
      : ESP_LOGE(TAG, "Register recv cb fail");
}

void TaskRadioMainLoop(void *pt)
{

  while (true)
  {
    switch (radio.status)
    {
    case RADIO_PAIR_DEVICE:
      // TODO 在规定时间内没有配对到设备时，发出错误提示
      if (radio.pairNewDevice() == ESP_OK)
        break;
      radio.status = RADIO_CONNECTED;
      break;

    case RADIO_BEFORE_CONNECTED:
      break;
    case RADIO_CONNECTED: // 连接成功开始传输数据
      // esp_err_t status = esp_now_send(slave.peer_addr,
      //                                 (uint8_t *)Presend_data,
      //                                 sizeof(&Presend_data));
      if (SENDCB(slave.peer_addr) != ESP_OK)
        ESP_LOGE(TAG, "data send fail");

      vTaskDelay(Send_gap_ms);
      break;

    case RADIO_BEFORE_DISCONNECT:
      break;

    case RADIO_DISCONNECT:
      break;

    default:
      vTaskDelay(5);
      break;
    }
  }
}

/**
 * @brief 启动无线
 * @param cb_fn 发送回调
 * @param send_gap_ms 数据同步间隔 /ms,最小值为 MinSendGapMs 定义的值
 */
void Radio::begin(send_cb_t cb_fn, int send_gap_ms)
{
  vehcile = &slave;
  SENDCB = cb_fn;
  Send_gap_ms = send_gap_ms <= MinSendGapMs ? MinSendGapMs : send_gap_ms;
  CONNECT_TIMEOUT = Send_gap_ms + 100; // 配置接收等待时间

  // EspNowInit();
  this->radioInit();
  this->status = RADIO_PAIR_DEVICE;

  ESP_LOGI(TAG, "SET All Task");
  xTaskCreate(TaskRadioMainLoop, "TaskRadioMainLoop", 4096, NULL, 2, NULL);

  ESP_LOGI(TAG, "Radio started :)");
}
