#include <radio.h>
#include <WiFi.h>
#include "esp_wifi.h"
#include "esp_wifi_types.h"
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

QueueHandle_t Q_ACK = xQueueCreate(2, sizeof(mac_t));

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

/**
 * @brief 等待主机握手
 * @param timeout 超时等待
 * @param data 收到响应时接收数据将写入其中
 * @param mac 过滤mac,非目标地址来的数据将被抛弃
 */
bool wait_response(TickType_t waitTick, radio_data_t *data, mac_t mac)
{
  radio_data_t temp_data;

  if (xQueueReceive(Q_RECV_DATA, &temp_data, waitTick) != pdPASS)
  {
    ESP_LOGI(TAG, "Wait for response timed out");
    return false;
  }
  if (temp_data.mac_addr != mac) // XXX 此递归机制有可能导致断联检测失效
    return wait_response(waitTick, data, mac);
  else
    memcpy(data, &temp_data, sizeof(temp_data));
  return true;
};

bool wait_ACK(TickType_t waitTick, mac_t mac)
{
  mac_t temp_mac;
  if (xQueueReceive(Q_ACK, &temp_mac, waitTick) != pdPASS)
  {
    ESP_LOGI(TAG, "Wait for ACK timed out");
    return false;
  }
  if (mac != temp_mac)
  {
    ESP_LOGI(TAG, "temp mac " MACSTR ", MAC " MACSTR "",
             MAC2STR(temp_mac), MAC2STR(mac));
    return wait_ACK(waitTick, mac);
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
 * @brief 添加对等设备
 * @param macaddr 数组 mac地址
 * @param channel wifi 频道
 * @param ifidx 要使用的wifi接口用于收发数据
 */
bool addPeer(
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
  case ESP_ERR_ESPNOW_EXIST:
    ESP_LOGE(TAG, "Peer Exists");
  case ESP_OK:
    RADIO.peer_info = peer_info;
    ESP_LOGI(TAG, "add peer success");
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
esp_err_t handshake(mac_t mac_addr)
{
  uint16_t timeOut = 50;
  radio_data_t data;
  mac_t newAddr; // 新地址
  if (!macOK(mac_addr))
    return false;

  // 2次握手请求，每次重试3次，均无响应握手失败
  for (size_t i = 1; i <= 3; i++) // M -> S
  {
    RADIO.send(data); // 不关心是否发送成功，当发送重试次数耗尽，判断握手失败
    if (wait_ACK(timeOut, mac_addr))
      for (size_t i = 1; i <= 3; i++) // S <- M
        if (wait_response(timeOut, &data))
          return ESP_OK;
  }

  return ESP_FAIL;
};

// 接收回调
void onRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  if (incomingData == nullptr)
    return;
  radio_data_t data;
  memcpy(&data, incomingData, sizeof(data));
  memcpy(&data.mac_addr, mac, sizeof(data.mac_addr));

  if (xQueueSend(Q_RECV_DATA, &data, 1) != pdPASS)
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

  if (status == ESP_NOW_SEND_SUCCESS)
  {
    counter_resend = 0;
    if (xQueueSend(Q_ACK, mac_addr, 1) != pdPASS)
      ;
  }
  else
  {
    ESP_LOGI(TAG, "Send to " MACSTR " FAIl", MAC2STR(mac_addr));
    counter_resend++;
  }

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
  static TickType_t last_run;
  static TickType_t temp;

  auto counter_max = 20;
  auto counter = 0;
  auto buffer = 0;
  while (true)
  {
    RADIO.update_loop_metrics();
    switch (RADIO.status)
    {
      /** 无连接状态，等待，什么都不做 */
    case RADIO_NO_CONNECTION_BEFORE:
      ESP_LOGI(TAG, "RADIO_NO_CONNECTION");
      RADIO.status = RADIO_NO_CONNECTION;
      break;

    case RADIO_NO_CONNECTION:
      vTaskDelay(RADIO.timeout_resend);
      break;

      /** 扫描AP */
    case RADIO_IN_SCAN_BEFORE:
      ESP_LOGI(TAG, "RADIO_IN_SCAN");
      RADIO.status = RADIO_IN_SCAN;
      break;
    case RADIO_IN_SCAN:
      RADIO.scan_ap();
      if (RADIO.status == RADIO_IN_SCAN)
        RADIO.status = RADIO_NO_CONNECTION_BEFORE;

      if (RADIO.adter_scan_ap_comp)
        RADIO.adter_scan_ap_comp(); // AP 扫描完成回调
      break;

    case RADIO_PAIR_DEVICE:
      if (RADIO.pairNewDevice() == ESP_OK)
        RADIO.status = RADIO_BEFORE_CONNECTED;
      else
        RADIO.status = RADIO_BEFORE_DISCONNECT;
      break;

    case RADIO_BEFORE_CONNECTED:
      RADIO.confgi_save();
      ESP_LOGI(TAG, "CONNECTED :)");
      RADIO.status = RADIO_CONNECTED;
      xLastWakeTime = xTaskGetTickCount();
      break;
    case RADIO_CONNECTED:
      counter++;
      if (RADIO.status != RADIO_CONNECTED)
        break;
      [&]() // 向接收机发送数据
      {
        RADIO.conected_before_send();
        radio_data_t data;
        xQueueReceive(Q_DATA_SEND, &data, 1);
        RADIO.send(data); // 回传数据
      }();

      [&]() // 等待回传数据，收到后通知外部程序处理
      {
        radio_data_t data;
        if (wait_response(RADIO.timeout_resend, &data))
          ;
        // if (xQueueSend(Q_DATA_RECV, &data, 2) == pdTRUE)
        // xQueueReset(Q_DATA_RECV); // 2 Tick 后队列依然满->清空队列
      }();

      temp = xTaskGetTickCount();
      buffer += (temp - last_run);
      last_run = temp;
      if (counter >= counter_max)
      {
        RADIO.run_time = buffer / 20;
        last_run = xTaskGetTickCount();
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
      if (RADIO.status != RADIO_DISCONNECT)
        break;

      if (macOK(RADIO.last_connected_device))
        if (handshake(RADIO.last_connected_device) == ESP_OK)
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
  auto tragetAP = &this->target_ap;

  // WiFi.begin(tragetAP->SSID);
  // while (WiFi.status() != WL_CONNECTED)
  // {
  //   ESP_LOGI(TAG, ".");
  //   delay(100);
  // }

  ESP_LOGI(TAG, "Connected to %s", tragetAP->SSID.c_str());
  //---------------- 握手 -----------------
  // 配对到从机 AP 地址&等待响应
  ESP_LOGI(TAG, "Pir to AP: %s", tragetAP->toStr().c_str());
  addPeer(tragetAP->MAC, tragetAP->CHANNEL, WIFI_IF_STA);
  if (handshake(tragetAP->MAC) == ESP_OK)
  {
    push_back(tragetAP->MAC);      // 添加已目标地址
    memcpy(&last_connected_device, // 设置最后连接的地址
           &RADIO.peer_info.peer_addr,
           sizeof(mac_t));
    ESP_LOGI(TAG, "" MACSTR " - HANDSHAKE SUCCESS", MAC2STR(tragetAP->MAC));
    return ESP_OK;
  }
  else
  {
    ESP_LOGI(TAG, "" MACSTR " - HANDSHAKE FIAL", MAC2STR(tragetAP->MAC));
    return ESP_FAIL;
  }
}

typedef struct
{
  uint8_t frame_ctrl[2];
  uint8_t duration_id[2];
  uint8_t addr1[6];
  uint8_t addr2[6];
  uint8_t addr3[6];
  uint8_t seq_ctrl[2];
  uint8_t addr4[6];
} wifi_ieee80211_mac_hdr_t;

typedef struct
{
  wifi_ieee80211_mac_hdr_t hdr;
  uint8_t payload[];
} wifi_ieee80211_packet_t;

/**
 * @brief  混杂模式监听回调,在这里读取来自从机的RSSI信号
 */
void promiscuous_rx_cb(void *buff, wifi_promiscuous_pkt_type_t type)
{
  if (type != WIFI_PKT_MGMT)
    return;

  const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;
  const wifi_ieee80211_packet_t *ipkt =
      (wifi_ieee80211_packet_t *)ppkt->payload;

  const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;

  mac_t source_mac = {
      hdr->addr2[0],
      hdr->addr2[1],
      hdr->addr2[2],
      hdr->addr2[3],
      hdr->addr2[4],
      hdr->addr2[5],
  };

  if (source_mac == RADIO.last_connected_device)
    RADIO.RSSI = ppkt->rx_ctrl.rssi;
  // ESP_LOGI(TAG, "Source MAC: " MACSTR "", MAC2STR(source_mac));
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

  // 开启混杂模式
  ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
  ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(&promiscuous_rx_cb));

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
  initRadio();
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

  this->status = RADIO_NO_CONNECTION_BEFORE;

  ESP_LOGI(TAG, "mac " MACSTR "", MAC2STR(last_connected_device));
  addPeer(last_connected_device, 1);

  xTaskCreatePinnedToCore(TaskRadioMainLoop, "TaskRadioMainLoop", 1024 * 10, NULL, 24, NULL, 0);

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

/**
 * @brief 扫描AP
 * @param v_array 扫描到的AP地址,扫描前会被清空
 * @return
 *  - ESP_OK 扫描完成并找到AP
 *  - ESP_FAIL 扫描完成没有找到符合条件的AP
 */
esp_err_t Radio::scan_ap()
{
  this->AP.clear();
  // 扫描1~13信道，过滤并储存所有查找到的AP信息
  for (size_t channel_i = 1; channel_i < 14; channel_i++)
  {
    auto scanResults = WiFi.scanNetworks(0, 0, 0, 100, channel_i);
    if (scanResults == 0) // 当前信道没有扫描到AP跳转到下一信道扫描
      continue;
    for (int i = 0; i < scanResults; i++)
      // 过滤 AP,由特定字符起始则被视为一个可以配对的设备
      if (WiFi.SSID(i).indexOf(SLAVE_KE_NAME) == 0)
        this->AP.emplace_back(ap_info(
            WiFi.SSID(i),
            WiFi.RSSI(i),
            WiFi.channel(i),
            WiFi.BSSID(i)));
    WiFi.scanDelete(); // 清除扫描信息
  }

  if (this->AP.size())
    return ESP_FAIL; // 没有找到AP
  return ESP_OK;
};

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

esp_err_t Radio::connect_to(const ap_info_t *apInfo)
{
  ESP_LOGI(TAG, "aa");
  RADIO.target_ap = *apInfo;
  this->status = RADIO_PAIR_DEVICE;
  return ESP_OK;
};