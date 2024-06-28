#include <radio.h>
#include <WiFi.h>
#include <vector>
#include <esp_wifi.h>
#include <algorithm>
#include <esp_err.h>
#include <cstring>
#include <esp_mac.h>
#include "tool.h"

#define TAG "Radio"

/******** 储存配置命名 ********/
#define STORGE_NAME_SPACE "RADIO_CONFIG" // 储存命名空间
#define STORGE_LAST_DEVICE "SLD"         // 最后连接的设备
#define STORGE_ALL_DEVICES "SAD"         // 配对过的设备

/**
 * AP扫描关键字
 *使用该字作为SSID起始将被视为一个可配对的设备
 * */
#define SLAVE_KE_NAME "Slave"

Radio RADIO;

// 接收数据队列, 用处理 esp_now 收到的数据
QueueHandle_t Q_RECV_DATA = xQueueCreate(2, sizeof(radio_data_t));

// 内部通知外部待处理数据
QueueHandle_t Q_DATA_RECV = xQueueCreate(2, sizeof(radio_data_t));

// 外部通知待发数据队列
QueueHandle_t Q_DATA_SEND = xQueueCreate(2, sizeof(radio_data_t));

/**
 * @brief 等待主机握手
 * @param timeout 超时等待
 * @param data 收到响应时接收数据将写入其中
 */
bool wait_response(TickType_t waitTick, radio_data_t *data)
{
  if (xQueueReceive(Q_RECV_DATA, data, waitTick) != pdPASS)
  {
    ESP_LOGI(TAG, "Wait for response timed out");
    return false;
  }
  return true;
};

// 对比 mac地址是否一致
bool areMacsEqual(const mac_t &mac1, const mac_t &mac2)
{
  return mac1 == mac2;
}

// 检查mac是否有效
bool macOK(const mac_t &arr)
{
  return std::any_of(arr.begin(), arr.end(),
                     [](uint8_t byte)
                     { return byte != 0x00 && byte != 0xFF; });
}

/**
 * @brief 根据 mac 地址配对到指定的设备
 * @param macaddr 数组 mac地址
 * @param channel wifi 频道
 * @param ifidx 要使用的wifi接口用于收发数据
 */
bool pairTo(
    const mac_t &macaddr,
    uint8_t channel,
    wifi_interface_t ifidx = WIFI_IF_STA)
{
  ESP_LOGI(TAG, "Pair to " MACSTR "", MAC2STR(macaddr));
  esp_now_peer_info_t peer_info;
  memcpy(peer_info.peer_addr, macaddr.data(), ESP_NOW_ETH_ALEN);
  peer_info.channel = channel;
  peer_info.encrypt = false;
  peer_info.ifidx = ifidx;

  switch (esp_now_add_peer(&peer_info))
  {
  case ESP_OK:
    RADIO.peer_info = peer_info;
    ESP_LOGI(TAG, "Pair success");
    return true;

  case ESP_ERR_ESPNOW_EXIST:
    RADIO.peer_info = peer_info;
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
 * @brief 与指定地址握手
 * @param mac_addr 目标地址
 */
bool handshake(mac_t mac_addr)
{
  radio_data_t data;
  mac_t newAddr; // 新地址
  if (!macOK(mac_addr))
    return false;

  if (!RADIO.send(data)) // 发送失败退出握手
    return false;
  // todo 验证响应地址

  if (!wait_response(RADIO.timeout_resend, &data))
    return false;

  // 当通道 0 有数据时表示响应设备将使用另一地址与主机通讯
  if (data.channel[0])
  {
    ESP_LOGI(TAG, "add new mac");
    if (esp_now_del_peer(RADIO.peer_info.peer_addr) != ESP_OK)
    {
      ESP_LOGE(TAG, "Failed to delete old peer");
      return false;
    }
    memcpy(newAddr.data(), &data.channel[0], newAddr.size());

    // XXX 信道固定
    if (!pairTo(newAddr, 1)) // 从通道2读取新信道
    {
      ESP_LOGE(TAG, "Failed to pair with new address");
      return false;
    }

    return handshake(newAddr); // 与新地址握手
  }

  // 对比收到的数据的发送地址与目标配对地址是否一致
  if (areMacsEqual(mac_addr, data.mac_addr))
    return true;
  else
  {
    ESP_LOGI(TAG, "handshake fail, T:" MACSTR ", R:" MACSTR "",
             MAC2STR(mac_addr), MAC2STR(data.mac_addr));
    return false;
  }
};

// 接收回调
void onRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  if (incomingData == nullptr)
    return;
  radio_data_t data;
  memcpy(&data, incomingData, sizeof(data));
  memcpy(&data.mac_addr, mac, sizeof(data.mac_addr));

  if (xQueueSend(Q_RECV_DATA, &data, 10) != pdPASS)
    ;
  // ESP_LOGI(TAG, "Queue is full.");
  // else
  //   ESP_LOGI(TAG, "Queue is add.");
  // ESP_LOGI(TAG, "Recv on " MACSTR "", MAC2STR(mac));
  // xTimerStop(ConnectTimeoutTimer, 10);
}

// 发送回调
void onSend(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  static uint8_t counter_resend = 0;

  if (status)
  {
    ESP_LOGI(TAG, "Send to " MACSTR " FAIl", MAC2STR(mac_addr));
    counter_resend++;
  }
  else
    counter_resend = 0;

  if (counter_resend >= RADIO.resend_count &&
      RADIO.status != RADIO_DISCONNECT &&
      RADIO.status != RADIO_BEFORE_DISCONNECT)
  {
    ESP_LOGI(TAG, "DISCONNECT with timeout");
    RADIO.status = RADIO_BEFORE_DISCONNECT;
    counter_resend = 0;
  }
  // else
  //   ESP_LOGI(TAG, "Send to " MACSTR " SUCCESS", MAC2STR(mac_addr));
}

// 主任务
void TaskRadioMainLoop(void *pt)
{
  const static TickType_t xFrequency = pdMS_TO_TICKS(8);
  static TickType_t xLastWakeTime = xTaskGetTickCount();
  static TickType_t start;
  static TickType_t end;

  uint8_t counter_pair_new_fail = 0;

  auto counter_max = 20;
  auto counter = 0;
  auto buffer = 0;
  while (true)
  {
    RADIO.update_loop_metrics();

    switch (RADIO.status)
    {
    case RADIO_PAIR_DEVICE:
      // TODO 在规定时间内没有配对到设备时，发出错误提示
      if (RADIO.pairNewDevice() != ESP_OK)
      {
        counter_pair_new_fail++;
        if (counter_pair_new_fail >= 100)
        {
          RADIO.status = RADIO_BEFORE_DISCONNECT;
          ESP_LOGI(TAG, "Pair New Devices Fail :(");
          break;
        }
      }
      else
        RADIO.status = RADIO_BEFORE_CONNECTED;
      break;

    case RADIO_BEFORE_CONNECTED:
      RADIO.confgi_save();
      ESP_LOGI(TAG, "CONNECTED :)");
      RADIO.status = RADIO_CONNECTED;
      xLastWakeTime = xTaskGetTickCount();
      break;

    case RADIO_CONNECTED:
      counter++;
      start = xTaskGetTickCount();

      if (RADIO.status != RADIO_CONNECTED)
        break;

      [&]() // 向接收机发送数据
      {
        radio_data_t data;
        xQueueReceive(Q_DATA_SEND, &data, 1);
        RADIO.send(data); // 回传数据
      }();

      [&]() // 等待回传数据，收到后通知外部程序处理
      {
        radio_data_t data;
        if (wait_response(RADIO.timeout_resend, &data))
          if (xQueueSend(Q_DATA_RECV, &data, 2) == pdTRUE)
            xQueueReset(Q_DATA_RECV); // 2 Tick 后队列依然满->清空队列
      }();

      end = xTaskGetTickCount();

      buffer += (end - start);
      if (counter >= counter_max)
      {
        RADIO.run_time = buffer / 20;
        end = xTaskGetTickCount();
        counter = 0;
        buffer = 0;
      }
      xTaskDelayUntil(&xLastWakeTime, xFrequency);
      break;

    case RADIO_BEFORE_DISCONNECT:
      ESP_LOGI(TAG, "RADIO_DISCONNECT");
      xQueueReset(Q_RECV_DATA); // 断开连接清空队列
      RADIO.status = RADIO_DISCONNECT;
      break;

    case RADIO_DISCONNECT:
      if (handshake(RADIO.last_connected_device))
        RADIO.status = RADIO_BEFORE_CONNECTED;
      vTaskDelay(RADIO.timeout_resend);
      break;

    default:
      esp_system_abort("Radio status error");
      break;
    }
  }
}

template <typename T>
bool Radio::send(const T &data)
{
  // ESP_LOGI(TAG, "Send to " MACSTR " - pd of : %d",
  //          MAC2STR(this->peer_info.peer_addr), &data);

  String error_message;
  switch (esp_now_send(this->peer_info.peer_addr,
                       (uint8_t *)&data, sizeof(data)))
  {
  case ESP_OK:
    return true;
  case ESP_ERR_ESPNOW_NO_MEM:
    error_message = String("out of memory");
    break;
  case ESP_ERR_ESPNOW_NOT_FOUND:
    error_message = String("peer is not found");
    break;

  case ESP_ERR_ESPNOW_IF:
    error_message = String("current WiFi interface doesn't match that of peer");
    break;

  case ESP_ERR_ESPNOW_NOT_INIT:
    error_message = String("ESPNOW is not initialized");
    break;

  case ESP_ERR_ESPNOW_ARG:
    error_message = String("invalid argument");
    break;

  case ESP_ERR_ESPNOW_INTERNAL:
    error_message = String("internal error");
    break;

  default:
    error_message = String("Send fail");
    break;
  }

  ESP_LOGE(TAG, "Send to " MACSTR " - %s",
           MAC2STR(this->peer_info.peer_addr), error_message.c_str());
  return false;
}

/**
 * @brief 配对新设备
 * @return esp_err_t ESP_OK/ESP_FAIL
 */
esp_err_t Radio::pairNewDevice()
{
  std::vector<ap_info> ap_infos(0);
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
  auto index = std::max_element(ap_infos.begin(), ap_infos.end()) - ap_infos.begin();
  auto tragetAP = &ap_infos[index];

  //---------------- 握手 -----------------
  // 配对到从机 AP 地址&等待响应
  ESP_LOGI(TAG, "Pir to AP: %s", tragetAP->toStr().c_str());

  pairTo(tragetAP->MAC, tragetAP->CHANNEL, WIFI_IF_STA);
  if (handshake(tragetAP->MAC))
    push_back(tragetAP->MAC);
  else
    return ESP_FAIL;
  // 设置最后连接的地址
  memcpy(&last_connected_device,
         &RADIO.peer_info.peer_addr,
         sizeof(mac_t));

  ESP_LOGI(TAG, "" MACSTR " - HANDSHAKE SUCCESS", MAC2STR(RADIO.peer_info.peer_addr));
  return ESP_OK;
}

// 初始化无线
void initRadio()
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
 */
void Radio::begin()
{
  // 读取配置
  nvs_call(STORGE_NAME_SPACE, [&](Preferences &prefs)
           {
             prefs
             .getBytes(
              STORGE_ALL_DEVICES,
               &paired_devices,
                sizeof(paired_devices));
             prefs
             .getBytes(
              STORGE_LAST_DEVICE,
               &last_connected_device,
                sizeof(mac_t)); });

  // 写入本机的 MAC 地址
  WiFi.macAddress(this->__mac_addr.data());

  initRadio();
  this->status = RADIO_DISCONNECT;

  ESP_LOGI(TAG, "mac " MACSTR "", MAC2STR(last_connected_device));
  pairTo(last_connected_device, 1);

  xTaskCreate(TaskRadioMainLoop, "TaskRadioMainLoop", 1024 * 10, NULL, 24, NULL);

  ESP_LOGI(TAG, "Radio started :)");
}

/**
 * @brief 启动无线
 * @param timeout 实际超时时间为 (timeout * send_gap) ms
 */
void Radio::begin(uint8_t timeout)
{
  this->timeout_resend = timeout;
}

/**
 * @brief 获取收到的数据
 */
esp_err_t Radio::get_data(radio_data_t *data)
{
  return xQueueReceive(Q_DATA_RECV, data, portMAX_DELAY) != pdPASS
             ? ESP_FAIL
             : ESP_OK;
}

/**
 * @brief 设置要发送的数据
 */
esp_err_t Radio::set_data(radio_data_t *data)
{
  return xQueueSend(Q_DATA_SEND, data, 5) != pdTRUE
             ? ESP_FAIL
             : ESP_OK;
}

void Radio::confgi_save()
{
  ESP_LOGI(TAG, "Save confgi");
  nvs_call(STORGE_NAME_SPACE, [&](Preferences &prefs)
           {
             prefs.putBytes(
              STORGE_LAST_DEVICE, 
              &last_connected_device,
               sizeof(mac_t));
             prefs.putBytes(
              STORGE_ALL_DEVICES,
               &paired_devices, 
               sizeof(paired_devices)); });
};

void Radio::config_clear()
{
  ESP_LOGI(TAG, "Clear confgi");
  nvs_call(STORGE_NAME_SPACE, [](Preferences &prefs)
           { prefs.clear(); });
}