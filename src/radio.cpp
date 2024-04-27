#include <esp_wifi.h>
#include <radio.h>
#include <WiFi.h>
#include <vector>
#include <esp_mac.h>

#define TAG "Radio"
#define SLAVE_KE_NAME "Slave"

String SSID;
int32_t RSSI;
String BSSIDstr;
int32_t CHANNEL;

esp_now_peer_info_t slave;

int CONNECT_TIMEOUT = 500;              // ms // 连接同步等待时间
int ConnectedTimeOut = CONNECT_TIMEOUT; // ms
const int MinSendGapMs = 8;
recvData Radio::RecvData;
bool PairRuning = false;
bool Radio::isPaired;
int Send_gap_ms = 0;
Radio radio;

send_cb_t SENDCB;

String parseMac(const uint8_t *mac)
{
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(macStr);
}

bool Pair(esp_now_peer_info_t slave)
{
  ESP_LOGI(TAG, "Pair to %s", parseMac(slave.peer_addr).c_str());
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
  AP_Info *temp_ap_info = nullptr;
  int16_t scanResults = 0;
  using namespace std;

  vector<vector<AP_Info>> ap_info;
  ap_info.resize(13);

  ESP_LOGI(TAG, "Start scan");
  // 遍历1~13信道
  for (size_t channel_i = 1; channel_i < 14; channel_i++)
  {
    scanResults = WiFi.scanNetworks(0, 0, 0, 100, channel_i);
    ap_info[channel_i - 1].resize(scanResults);
    if (scanResults == 0) // 当前信道没有扫描到设备跳转到下一信道扫描
      continue;

    for (int i = 0; i < scanResults; i++)
    {
      temp_ap_info = &ap_info[channel_i - 1][i];
      temp_ap_info->CHANNEL = WiFi.channel(i);
      temp_ap_info->SSID = WiFi.SSID(i);
      temp_ap_info->RSSI = WiFi.RSSI(i);
      memcpy(temp_ap_info->MAC, WiFi.BSSIDstr(i).c_str(), ESP_NOW_ETH_ALEN);

      // 检索特定SSID名称以及信号强度，满足要求则开始配对
      if (temp_ap_info->SSID.indexOf(SLAVE_KE_NAME) == 0)
      {
        ESP_LOGI(TAG, "SSID: %s, MAC:" MACSTR ", RSSI: %d, Channel: %d", temp_ap_info->SSID, MAC2STR(temp_ap_info->MAC), temp_ap_info->RSSI, temp_ap_info->CHANNEL);
      }
    }
  }
  WiFi.scanDelete(); // 清除扫描信息
  ESP_LOGI(TAG, "COMP");
  return ESP_OK;
}

void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  ConnectedTimeOut = CONNECT_TIMEOUT; // 接收到数据时重置超时
  memcpy(&Radio::RecvData, incomingData, sizeof(Radio::RecvData));

  // ESP_LOGI(TAG, "Recv data from : %s, mv:%d", parseMac(mac).c_str(), Radio::RecvData.mv);

  if (PairRuning)
  {
    ESP_LOGI(TAG, "Recv data from : %s", parseMac(mac).c_str());

    ConnectedTimeOut = CONNECT_TIMEOUT; // 接收到数据时重置超时
    slave.channel = CHANNEL;
    slave.encrypt = 0;
    memcpy(slave.peer_addr, mac, 6);

    if (Pair(slave))
    {
      PairRuning = false;
      Radio::isPaired = true;
    }
  }
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

void TaskSend(void *pt)
{
  while (true)
  {
    if (Radio::isPaired)
    {
      esp_err_t status = SENDCB(slave.peer_addr);
      // esp_err_t status = esp_now_send(slave.peer_addr,
      //                                 (uint8_t *)Presend_data,
      //                                 sizeof(&Presend_data));
      if (status != ESP_OK)
        ESP_LOGE(TAG, "data send fail");
    }
    vTaskDelay(Send_gap_ms);
  }
}

void TaskRadioMainLoop(void *pt)
{

  while (true)
  {
    switch (radio.status)
    {
    case RADIO_PAIR_DEVICE:
      radio.pairNewDevice();
      vTaskDelay(100);
      break;

    case RADIO_BEFORE_CONNECTED:
      break;
    case RADIO_CONNECTED:
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
  isPaired = false;
  vehcile = &slave;
  SENDCB = cb_fn;
  Send_gap_ms = send_gap_ms <= MinSendGapMs ? MinSendGapMs : send_gap_ms;
  CONNECT_TIMEOUT = Send_gap_ms + 100; // 配置接收等待时间

  // EspNowInit();
  this->radioInit();
  this->status = RADIO_PAIR_DEVICE;

  ESP_LOGI(TAG, "SET All Task");
  // xTaskCreate(TaskSend, "TaskSend", 2048, NULL, 2, NULL);

  xTaskCreate(TaskRadioMainLoop, "TaskRadioMainLoop", 4096, NULL, 2, NULL);

  ESP_LOGI(TAG, "Radio started :)");
}

/**
 * @brief 获取配对状态
 */
bool Radio::getPairStatus()
{
  return isPaired;
}