#include <radio.h>
#include <WiFi.h>
#include <vector>
#include <esp_wifi.h>
#include <algorithm>
#include <esp_err.h>
#include <cstring>
#include <esp_mac.h>

#define TAG "Radio"

/**
 * AP扫描关键字
 *使用该字作为SSID起始将被视为一个可配对的设备
 * */
#define SLAVE_KE_NAME "Slave"
using namespace std;
bool SEND_READY = false; // 允许发送数据

esp_now_peer_info_t slave;
Radio radio;

// 连接超时控制器
TimerHandle_t ConnectTimeoutTimer;
const int ConnectTimeoutTimerID = 0;

/**
 * @brief 阻塞等待
 * @param timeout 最大等待时间，超时返回 false
 * @param falg false等待/true结束等待并返回true
 */
bool waitTimeout(uint timeout, bool *flag)
{
  uint counter = 0;
  while (!*flag)
  {
    if (counter >= timeout)
      return false;
    counter++;
    vTaskDelay(1);
  }
  return true;
};

// 连接超时控制器回调
void IfTimeoutCB(TimerHandle_t xTimer)
{
  ESP_LOGI(TAG, "Wait for response timed out");
  radio.status = RADIO_BEFORE_DISCONNECT;
  if (xTimerStop(ConnectTimeoutTimer, 10) != pdPASS)
    esp_system_abort("stop timer fial"); // 停止定时器失败
  else
    ESP_LOGI(TAG, "Timer stop");
}

/**
 * @brief 根据 mac 地址配对到指定的设备
 * @param macaddr 数组 mac地址
 * @param channel wifi 频道
 * @param ifidx 要使用的wifi接口用于收发数据
 */
bool pairTo(
    uint8_t macaddr[ESP_NOW_ETH_ALEN],
    uint8_t channel,
    wifi_interface_t ifidx)
{
  // ESP_LOGI(TAG, "Pair to " MACSTR "", MAC2STR(macaddr));
  memset(&slave, 0, sizeof(esp_now_peer_info_t)); // 清空对象
  memcpy(slave.peer_addr, macaddr, ESP_NOW_ETH_ALEN);
  slave.channel = channel;
  slave.ifidx = ifidx;
  slave.encrypt = false;

  esp_err_t addStatus = esp_now_add_peer(&slave);

  switch (addStatus)
  {
  case ESP_OK:
    // ESP_LOGI(TAG, "Pair success");
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

bool sendTo(const uint8_t *peer_addr, const uint8_t *data, size_t len)
{
  auto status = esp_now_send(peer_addr, data, len);
  String error_message;

  if (status == ESP_OK)
    return true;

  switch (status)
  {
  case ESP_ERR_ESPNOW_NO_MEM:
    error_message = String("out of memory");
  case ESP_ERR_ESPNOW_NOT_FOUND:
    error_message = String("peer is not found");
  case ESP_ERR_ESPNOW_IF:
    error_message = String("current WiFi interface doesn't match that of peer");
  default:
    error_message = String("Send fail");
  }

  ESP_LOGI(TAG, "Send to " MACSTR " - %s",
           error_message.c_str(), MAC2STR(peer_addr));
  return false;
};

/**
 * @brief 与指定地址握手
 */
bool handshake(uint8_t macaddr[ESP_NOW_ETH_ALEN], HANDSHAKE_DATA *recv_hsd)
{
  uint8_t timeout = 50; // XXX 从 radio 对象获取值
  HANDSHAKE_DATA HSD;
  WiFi.macAddress(HSD.mac);
  ESP_LOGI(TAG, "mac :" MACSTR " code : %u", MAC2STR(HSD.mac), HSD.code);
  sendTo(macaddr, (uint8_t *)&HSD, sizeof(HSD));
  if (!waitTimeout(timeout, &radio.RecvData.newData))
  {
    ESP_LOGI(TAG, "Wait for response timed out");
    return false;
  }

  memset(recv_hsd, 0, sizeof(HANDSHAKE_DATA));
  memcpy(recv_hsd, radio.RecvData.get(), sizeof(HANDSHAKE_DATA));
  if (HSD.code == recv_hsd->code)
    return ESP_OK;

  ESP_LOGI(TAG, "Checksum is inconsistent M:%u - S:%u",
           HSD.code, recv_hsd->code);
  return ESP_FAIL; // 配对码不一致则判断配对失败
};

/**
 * @brief 配对新设备
 * @return esp_err_t ESP_OK/ESP_FAIL
 */
esp_err_t Radio::pairNewDevice()
{
  vector<AP_Info> ap_info(0);
  int16_t scanResults = 0;
  HANDSHAKE_DATA recv_data; // 接收数据包

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
  auto tragetAP =
      &ap_info[max_element(ap_info.begin(), ap_info.end()) - ap_info.begin()];
  ESP_LOGI(TAG, "Target AP : %s", tragetAP->toStr().c_str());

  //---------------- 握手 -----------------
  // 配对到从机 AP 地址&等待响应
  ESP_LOGI(TAG, "Pir to AP");
  pairTo(tragetAP->MAC, tragetAP->CHANNEL, WIFI_IF_STA);
  if (handshake(tragetAP->MAC, &recv_data))
  {
    ESP_LOGI(TAG, "AP " MACSTR " - HANDSHAKE FAIL", MAC2STR(tragetAP->MAC));
    return ESP_FAIL;
  }
  ESP_LOGI(TAG, "AP " MACSTR " - HANDSHAKE SUCCESS", MAC2STR(tragetAP->MAC));

  // 配对到从机 STA 地址&等待响应
  ESP_LOGI(TAG, "Pir to STA");
  pairTo(radio.RecvData.mac, slave.channel, WIFI_IF_STA);
  if (handshake(recv_data.mac, &recv_data))
  {
    ESP_LOGI(TAG, "STA " MACSTR " - HANDSHAKE FAIL"), MAC2STR(recv_data.mac);
    return ESP_FAIL;
  }
  ESP_LOGI(TAG, "STA " MACSTR " - HANDSHAKE SUCCESS", MAC2STR(recv_data.mac));

  ESP_LOGI(TAG, "COMP");
  return ESP_OK;
}

// 接收回调
void onRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  radio.RecvData.len = len;
  radio.RecvData.newData = true;
  radio.RecvData.mac = (uint8_t *)mac;
  radio.RecvData.incomingData = (uint8_t *)incomingData;
  SEND_READY = true; // 收到数据后允许发送
  // ESP_LOGI(TAG, "Recv on " MACSTR "", MAC2STR(mac));
  xTimerStop(ConnectTimeoutTimer, 10);
}

// 发送回调
void onSend(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  // SEND_READY = false;
  // if (status)
  //   ESP_LOGI(TAG, "Send to " MACSTR " FAIl", MAC2STR(mac_addr));
  // else
  // ESP_LOGI(TAG, "Send to " MACSTR " SUCCESS", MAC2STR(mac_addr));
  xTimerStart(ConnectTimeoutTimer, 10);
}

// 主任务
void TaskRadioMainLoop(void *pt)
{
  vTaskDelay(50);              // 延迟启动循环
  uint8_t timeout_counter = 0; // 连接超时计数器

  while (true)
  {
    switch (radio.status)
    {
    case RADIO_PAIR_DEVICE:
      // TODO 在规定时间内没有配对到设备时，发出错误提示
      if (radio.pairNewDevice() != ESP_OK)
      {
        radio.status = RADIO_BEFORE_DISCONNECT;
        ESP_LOGI(TAG, "Pair New Devices Fail :(");
        break;
      }
      radio.status = RADIO_BEFORE_CONNECTED;
      break;

    case RADIO_BEFORE_CONNECTED:
      ESP_LOGI(TAG, "CONNECTED :)");
      radio.status = RADIO_CONNECTED;
      SEND_READY = true;
      if (xTimerStart(ConnectTimeoutTimer, 10) != pdPASS)
        esp_restart();
      break;

    case RADIO_CONNECTED:
      // 等待发送窗口超时，视为连接断开
      SEND_READY ? timeout_counter = 0 : timeout_counter++;
      if (timeout_counter >= radio.timeOut)
      {
        ESP_LOGI(TAG, "DISCONNECT with timeout");
        radio.status = RADIO_BEFORE_DISCONNECT;
        break;
      }

      if (SEND_READY)
      {
        SEND_READY = false;
        sendTo(slave.peer_addr, radio.dataToSent, sizeof(radio.dataToSent));
      }

      vTaskDelay(radio.sendGap);
      break;

    case RADIO_BEFORE_DISCONNECT:
      ESP_LOGI(TAG, "RADIO_BEFORE_DISCONNECT");
      radio.status = RADIO_DISCONNECT;
      break;

    case RADIO_DISCONNECT:
      vTaskDelay(100);
      break;

    default:
      ESP_LOGI(TAG, "ERROR  status! esp_restart");
      esp_restart();
      vTaskDelay(5);
      break;
    }
  }
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
  esp_now_register_recv_cb(onRecv) == ESP_OK
      ? ESP_LOGI(TAG, "Register recv cb success")
      : ESP_LOGE(TAG, "Register recv cb fail");

  // 注册发送回调
  esp_now_register_send_cb(onSend) == ESP_OK
      ? ESP_LOGI(TAG, "Register recv cb success")
      : ESP_LOGE(TAG, "Register recv cb fail");
}

/**
 * @brief 启动无线
 * @param data_to_sent 要发送的数据指针
 */
void Radio::begin(uint8_t *data_to_sent)
{

  // 定义连接超时控制器
  ConnectTimeoutTimer = xTimerCreate(
      "Connect time out",             // 定时器任务名称
      this->sendGap * this->timeOut,  // 延迟多少tick后执行回调函数
      pdTRUE,                         // 执行一次,pdTRUE 循环执行
      (void *)&ConnectTimeoutTimerID, // 任务id
      IfTimeoutCB                     // 回调函数
  );

  vehcile = &slave;
  this->dataToSent = data_to_sent;

  this->radioInit();
  this->status = RADIO_DISCONNECT;

  xTaskCreate(TaskRadioMainLoop, "TaskRadioMainLoop", 4096, NULL, 2, NULL);

  ESP_LOGI(TAG, "Radio started :)");
}

/**
 * @brief 启动无线
 * @param data_to_sent 要发送的数据指针
 * @param timeout 实际超时时间为 (timeout * send_gap) ms
 * @param send_gap 要发送的数据指针 数据发送间隔
 */
void Radio::begin(uint8_t *data_to_sent, uint8_t timeout, uint8_t send_gap)
{
  this->timeOut = timeout;
  this->sendGap = sendGap;
  this->begin(data_to_sent);
}
