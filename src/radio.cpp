#include <radio.h>
#include <WiFi.h>
#include <vector>
#include <esp_wifi.h>
#include <algorithm>
#include <esp_err.h>
#include <cstring>
#include <esp_mac.h>

#define TAG "Radio"
#define SLAVE_KE_NAME "Slave"

using namespace std;

esp_now_peer_info_t slave;
const int MinSendGapMs = 8;
int Send_gap_ms = 0;
Radio radio;

send_cb_t SENDCB;

// 配对到设备
bool pairTo(AP_Info *apInfo)
{
  ESP_LOGI(TAG, "Pair to " MACSTR "", MAC2STR(apInfo->MAC));
  memset(&slave, 0, sizeof(esp_now_peer_info_t)); // 清空对象
  memcpy(slave.peer_addr, apInfo->MAC, ESP_NOW_ETH_ALEN);
  slave.channel = apInfo->CHANNEL;
  slave.ifidx = WIFI_IF_STA;
  slave.encrypt = false;

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

  // 打印所有扫描到的 AP
  for (size_t i = 0; i < ap_info.size(); i++)
    ESP_LOGI(TAG, "%s", ap_info[i].toStr().c_str());

  ESP_LOGI(TAG, "AP scan comp");

  if (ap_info.size() < 1) // 没有AP被扫描到
    return ESP_FAIL;

  // 查找信号最强的AP并与其配对
  int8_t targetIndex =
      max_element(ap_info.begin(), ap_info.end()) - ap_info.begin();

  ESP_LOGI(TAG, "Target -- %s", ap_info[targetIndex].toStr().c_str());

  pairTo(&ap_info[targetIndex]);

  const char data[6] = "Hello";
  esp_now_send(slave.peer_addr, (uint8_t *)&data, sizeof(data));

  ESP_LOGI(TAG, "COMP");
  return ESP_OK;
}

// 将接收到的数据更新到对象
void onRecvCb(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  radio.RecvData.len = len;
  radio.RecvData.newData = true;
  radio.RecvData.mac = (uint8_t *)mac;
  radio.RecvData.incomingData = (uint8_t *)incomingData;
}

// 初始化无线
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
  esp_now_register_recv_cb(onRecvCb) == ESP_OK
      ? ESP_LOGI(TAG, "Register recv cb success")
      : ESP_LOGE(TAG, "Register recv cb fail");
}

// 主任务
void TaskRadioMainLoop(void *pt)
{
  vTaskDelay(50); // 延迟启动循环

  while (true)
  {
    switch (radio.status)
    {
    case RADIO_PAIR_DEVICE:
      // TODO 在规定时间内没有配对到设备时，发出错误提示
      if (radio.pairNewDevice() == ESP_OK)
        radio.status = RADIO_CONNECTED;
      else
      {
        ESP_LOGI(TAG, "Pair New Devices Fail :(");
        continue;
      }

      break;

    case RADIO_BEFORE_CONNECTED:
      break;
    case RADIO_CONNECTED: // 连接成功开始传输数据

      // TODO 传入 一个待发送数据的指针，而不是 使用回调函数控制发送的数据
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

  // EspNowInit();
  this->radioInit();
  this->status = RADIO_PAIR_DEVICE;

  ESP_LOGI(TAG, "SET All Task");
  xTaskCreate(TaskRadioMainLoop, "TaskRadioMainLoop", 4096, NULL, 2, NULL);

  ESP_LOGI(TAG, "Radio started :)");
}
