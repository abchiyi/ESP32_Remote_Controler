#include <esp_now.h>
#include <esp_wifi.h>
#include <radio.h>
#include <WiFi.h>

#define CONNECT_TIMEOUT 200 // ms // 连接同步等待时间
#define TAG "Radio"
String SSID;
int32_t RSSI;
String BSSIDstr;
int32_t CHANNEL;
uint8_t *staMac;

bool IsPaired = false;
void *presend_data;
int32_t send_gap_ms;

// 通讯超时设置
int ConnectedTimeOut = CONNECT_TIMEOUT; // ms

esp_now_peer_info_t slave;

String parseMac(const uint8_t *mac)
{
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(macStr);
}

void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print("Last Packet Sent to: ");
  Serial.println(macStr);
  Serial.print("Last Packet Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if (status == ESP_NOW_SEND_SUCCESS)
  {
    ConnectedTimeOut = CONNECT_TIMEOUT; // 发送成功重置超时
  }
  Serial.print("This Mac :");
  Serial.println(WiFi.macAddress());
}

void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  static int conter = 0;
  conter++;
  ConnectedTimeOut = CONNECT_TIMEOUT; // 接收到数据时重置超时
  ESP_LOGI(TAG, "%u :Recv data from : %s", conter, parseMac(mac).c_str());
}

/* 检查连接同步状态，
一段时间内没有接收到回复信息，
或连续数据发送失败则认为连接已断开 */
void TaskConnectedWatch(void *pt)
{
  while (true)
  {
    bool oldIsPaired = IsPaired;

    if (IsPaired) // 连接成功时开始倒计时，超时设置状态为 “未连接”
    {
      ConnectedTimeOut--;
      if (ConnectedTimeOut <= 0)
      {
        IsPaired = false;
        ESP_LOGI(TAG, "Lost connection");
      }
    }
    else
    {
      if (IsPaired != oldIsPaired)
      {
        ESP_LOGI(TAG, "---------------Time Out--------------");
        oldIsPaired = IsPaired;
        ESP_LOGI(TAG, "reset");
        ConnectedTimeOut = CONNECT_TIMEOUT;
        esp_now_unregister_recv_cb();
      }
    }
    vTaskDelay(1);
  }
}

// 此回调用于配对
void PeerDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  // 删除之前配对的AP mac
  esp_now_del_peer(slave.peer_addr);

  slave.channel = CHANNEL;
  slave.encrypt = 0;
  memcpy(slave.peer_addr, mac, 6);
  esp_err_t addStatus = esp_now_add_peer(&slave);
  if (addStatus == ESP_OK || addStatus == ESP_ERR_ESPNOW_EXIST)
  {
    IsPaired = true;
    ESP_LOGI(TAG, "Recv data from : %s", parseMac(mac).c_str());
    ESP_LOGI(TAG, "Peer success ");
    // 配对完成注册工作函数
    esp_err_t a = esp_now_register_recv_cb(onDataRecv);
    esp_err_t b = esp_now_register_send_cb(onDataSent);
    if (a == ESP_OK && b == ESP_OK)
    {
      ESP_LOGI(TAG, "register func success ");
    }
    else
    {
      ESP_LOGE(TAG, "register func fail");
    }
  }
  else
  {
    ESP_LOGE(TAG, "Add peed Fail");
  }

  esp_now_unregister_recv_cb(); // 此回调在配对完成后删除
}

/**
 * @brief 扫描&配对从机,检测  IsPaired 的状态，当未配对时将开始扫描并配对设备
 */
void TaskScanAndPeer(void *pt)
{

  while (true)
  {
    bool findSlave = false;
    if (!IsPaired)
    {
      // ESP_LOGI(TAG, "Start scan");
      int16_t scanResults = WiFi.scanNetworks(0, 0, 0, 50, 1); // 扫描1通道
      if (scanResults == 0)
      {
        ESP_LOGI(TAG, "No WiFi devices in AP Mode found,Try again");
        continue; // 跳过剩余 code 立即重新扫描
      }
      else
      {
        memset(&slave, 0, sizeof(slave));
        for (int i = 0; i < scanResults; i++)
        {
          SSID = WiFi.SSID(i);
          RSSI = WiFi.RSSI(i);
          BSSIDstr = WiFi.BSSIDstr(i);
          CHANNEL = WiFi.channel(i);

          // Check if the current device starts with `Slave`
          if (SSID.indexOf("Slave") == 0)
          {
            findSlave = true;
            // SSID of interest
            ESP_LOGI(TAG, "Found a Slave : %u : SSID: %s, BSSID: [%s], RSSI:  (%lu), Channel: %lu", i + 1, SSID, BSSIDstr.c_str(), RSSI, CHANNEL);

            // 设置从机配对信息
            // Get BSSID => Mac Address of the Slave
            int mac[6];
            sscanf(
                BSSIDstr.c_str(), "%x:%x:%x:%x:%x:%x",
                &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
            for (int v = 0; v < 6; ++v)
            {
              slave.peer_addr[v] = (uint8_t)mac[v];
            }

            // use slave ap channel
            slave.channel = CHANNEL;
            // no encryption
            slave.encrypt = 0;
            // 匹配信道
            esp_wifi_set_channel(CHANNEL, WIFI_SECOND_CHAN_NONE);

            WiFi.scanDelete(); // 清除扫描信息
          }
          break; // 获取到第一个从机信息时结束for循环
        }
      }

      if (findSlave)
      {
        ESP_LOGI(TAG, "To Peer");
        esp_err_t addStatus = esp_now_add_peer(&slave);

        // 配对成功 or 配对已存在即配对成功，否则重新循环并扫描配对AP
        if (addStatus == ESP_OK || addStatus == ESP_ERR_ESPNOW_EXIST)
        {
          uint8_t data = 0;
          ESP_LOGI(TAG, "Send peer message to %s", SSID.c_str());
          esp_now_send(slave.peer_addr, &data, sizeof(data));

          esp_now_register_recv_cb(PeerDataRecv); // 此回调用于配对
          int conter = CONNECT_TIMEOUT;
          while (!IsPaired) // 等待从机发回 STA 模式 mac 地址
          {
            if (conter < 1)
            {
              ESP_LOGE(TAG, "Peer fail");
              // 取消注册PeerDataRecv 该函数仅用于配对
              esp_now_unregister_recv_cb();
              break;
            }
            vTaskDelay(1);
            conter--;
          }
        }
        else
        {
          ESP_LOGE(TAG, "Peer fail");
        }
      }
    }
    else
    {
      vTaskDelay(100);
    }
  }
}

/**
 * @brief 开启 esp now 连接
 * @param presend_data 需要同步的数据的指针
 * @param send_gap_ms 数据同步间隔 /ms,为 0 则不设置间隔
 */
void Radio::begin(void *presend_data, int send_gap_ms)
{
  isPaired = &IsPaired;
  vehcile = &slave;

  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_now_init() == ESP_OK
      ? ESP_LOGI(TAG, "ESPNow Init Success")
      : ESP_LOGE(TAG, "ESPNow Init Failed...restarting...");
  ESP_LOGI(TAG, "STA MAC: %s, STA CHANNEL: %u", WiFi.macAddress().c_str(), WiFi.channel());

  ESP_LOGI(TAG, "SET All Task");
  xTaskCreate(TaskConnectedWatch, "TaskConnectedWatch", 2048, NULL, 2, NULL);
  xTaskCreate(TaskScanAndPeer, "TaskScanAndPeer", 4096, NULL, 2, NULL);
}

/**
 * @brief 获取配对状态
 */
bool Radio::getPairStatus()
{
  return *isPaired;
}