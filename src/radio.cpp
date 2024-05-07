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

Radio radio;

QueueHandle_t Q_RECV_DATA = xQueueCreate(10, sizeof(radio_data_t));

SemaphoreHandle_t NEW_DATA = NULL;         // 收到数据
SemaphoreHandle_t SEND_READY = NULL;       // 允许发送
SemaphoreHandle_t CAN_CONNECT_LAST = NULL; // 连接到最后一次通讯的地址

TimerHandle_t ConnectTimeoutTimer;
const int ConnectTimeoutTimerID = 0;

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
    mac_addr_t macaddr,
    uint8_t channel,
    wifi_interface_t ifidx = WIFI_IF_STA)
{
  // ESP_LOGI(TAG, "Pair to " MACSTR "", MAC2STR(macaddr));
  auto peer_info = &radio.peer_info;
  memset(peer_info, 0, sizeof(esp_now_peer_info_t)); // 清空对象
  memcpy(peer_info->peer_addr, macaddr, ESP_NOW_ETH_ALEN);
  peer_info->channel = channel;
  peer_info->encrypt = false;
  peer_info->ifidx = ifidx;

  esp_err_t addStatus = esp_now_add_peer(peer_info);

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

template <typename T>
bool Radio::send(const T &data)
{
  auto status = esp_now_send(this->peer_info.peer_addr,
                             (uint8_t *)&data, sizeof(data));
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
           error_message.c_str(), MAC2STR(this->peer_info.peer_addr));
  return false;
}

/**
 * @brief 与指定地址握手
 */
bool handshake(mac_addr_t mac_addr)
{
  radio_data_t data;
  mac_addr_t newAddr; // 新地址

  radio.send(data); // todo 验证响应地址

  if (xQueueReceive(Q_RECV_DATA, &data, radio.timeOut) != pdPASS)
  {
    ESP_LOGI(TAG, "Wait for response timed out");
    return false;
  }

  // 当通道 0 有数据时表示响应设备将使用另一地址与主机通讯
  if (data.channel[0])
  {
    ESP_LOGI(TAG, "add new mac");
    esp_now_del_peer(radio.peer_info.peer_addr);
    memcpy(newAddr, &data.channel[0], sizeof(mac_addr_t));
    pairTo(newAddr, 1);                          // 从通道2读取新信道
    return handshake(radio.peer_info.peer_addr); // 与新地址握手
  }

  // 对比收到的数据的发送地址与目标配对地址是否一致
  if (memcmp(mac_addr, data.mac_addr, sizeof(mac_addr_t)) == 0)
    return true;
  else
  {
    ESP_LOGI(TAG, "handshake fail, T:" MACSTR ", R:" MACSTR "",
             MAC2STR(mac_addr), MAC2STR(data.mac_addr));
    return false;
  }
};

/**
 * @brief 配对新设备
 * @return esp_err_t ESP_OK/ESP_FAIL
 */
esp_err_t Radio::pairNewDevice()
{
  vector<ap_info> ap_infos(0);
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
        ap_infos.emplace_back(ap_info(
            WiFi.SSID(i),
            WiFi.RSSI(i),
            WiFi.channel(i),
            WiFi.BSSID(i)));
    vTaskDelay(1);
    WiFi.scanDelete(); // 清除扫描信息
  }

  // 打印所有扫描到的 AP
  for (size_t i = 0; i < ap_infos.size(); i++)
    ESP_LOGI(TAG, "%s", ap_infos[i].toStr().c_str());

  ESP_LOGI(TAG, "AP scan comp");

  if (ap_infos.size() < 1) // 没有AP被扫描到
    return ESP_FAIL;

  // 查找信号最强的AP并与其配对
  auto tragetAP =
      &ap_infos[max_element(ap_infos.begin(), ap_infos.end()) - ap_infos.begin()];
  ESP_LOGI(TAG, "Target AP : %s", tragetAP->toStr().c_str());

  //---------------- 握手 -----------------
  // 配对到从机 AP 地址&等待响应
  ESP_LOGI(TAG, "Pir to AP");
  pairTo(tragetAP->MAC, tragetAP->CHANNEL, WIFI_IF_STA);
  if (!handshake(tragetAP->MAC))
    return ESP_FAIL;
  ESP_LOGI(TAG, "" MACSTR " - HANDSHAKE SUCCESS",
           MAC2STR(radio.peer_info.peer_addr));
  return ESP_OK;
}

// 接收回调
void onRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  radio_data data;
  memcpy(&data, incomingData, sizeof(data));
  memcpy(&data.mac_addr, mac, sizeof(data.mac_addr));

  if (xQueueSend(Q_RECV_DATA, &data, 10) != pdPASS)
    ;
  // ESP_LOGI(TAG, "Queue is full.");
  // else
  //   ESP_LOGI(TAG, "Queue is add.");

  xSemaphoreGive(SEND_READY); // 收到数据后允许发送
  xSemaphoreGive(NEW_DATA);

  // ESP_LOGI(TAG, "Recv on " MACSTR "", MAC2STR(mac));
  // xTimerStop(ConnectTimeoutTimer, 10);
}

// 发送回调
void onSend(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  // SEND_READY = false;
  // if (status)
  //   ESP_LOGI(TAG, "Send to " MACSTR " FAIl", MAC2STR(mac_addr));
  // else
  // ESP_LOGI(TAG, "Send to " MACSTR " SUCCESS", MAC2STR(mac_addr));
  // xTimerStart(ConnectTimeoutTimer, 10);
}

// 主任务
void TaskRadioMainLoop(void *pt)
{
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
      xSemaphoreGive(CAN_CONNECT_LAST);
      ESP_LOGI(TAG, "CONNECTED :)");
      radio.status = RADIO_CONNECTED;
      // if (xTimerStart(ConnectTimeoutTimer, 10) != pdPASS)
      //   esp_restart();
      break;

    case RADIO_CONNECTED:
      if (xSemaphoreTake(SEND_READY, radio.timeOut) == pdTRUE)
      {
        radio.send(radio.dataToSent);
        break;
      }

      // TODO 通讯多次超时判断连接断开
      ESP_LOGI(TAG, "DISCONNECT with timeout");
      radio.status = RADIO_BEFORE_DISCONNECT;
      break;

    case RADIO_BEFORE_DISCONNECT:
      xQueueReset(Q_RECV_DATA); // 断开连接清空队列
      radio.status = RADIO_DISCONNECT;
      xSemaphoreGive(CAN_CONNECT_LAST);
      break;

    case RADIO_DISCONNECT:
      if (xSemaphoreTake(CAN_CONNECT_LAST, radio.timeOut) == pdTRUE)
        if (handshake(radio.peer_info.peer_addr))
          radio.status = RADIO_BEFORE_CONNECTED;
        else
          xSemaphoreGive(CAN_CONNECT_LAST);
      vTaskDelay(radio.timeOut);
      break;

    default:
      esp_system_abort("Radio status error");
      break;
    }
  }
}

// 初始化无线
void Radio::initRadio()
{
  // set wifi
  ESP_LOGI(TAG, "Init wifi");
  WiFi.enableLongRange(true);
  // esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR);
  // esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_LR);

  // 设置模式 STA
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
      ? ESP_LOGI(TAG, "Register send cb success")
      : ESP_LOGE(TAG, "Register send cb fail");
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

  // 写入本机的 MAC 地址
  WiFi.macAddress(this->__mac_addr);

  // 创建信号量
  SEND_READY = xSemaphoreCreateBinary();
  NEW_DATA = xSemaphoreCreateBinary();
  CAN_CONNECT_LAST = xSemaphoreCreateBinary();

  this->initRadio();
  this->status = RADIO_DISCONNECT;

  xTaskCreate(TaskRadioMainLoop, "TaskRadioMainLoop", 1024 * 10, NULL, 2, NULL);

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
