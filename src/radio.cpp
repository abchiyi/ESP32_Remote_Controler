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

bool PairRuning = false;
bool IsPaired = false;
void *Presend_data;
int32_t Send_gap_ms = 0;

// 通讯超时设置
int ConnectedTimeOut = CONNECT_TIMEOUT; // ms

esp_now_peer_info_t slave;
void TaskSend(void *pt)
{
  while (true)
  {
    if (IsPaired)
    {
      esp_err_t status = esp_now_send(slave.peer_addr,
                                      (uint8_t *)Presend_data,
                                      sizeof(&Presend_data));
      if (status != ESP_OK)
      {
        ESP_LOGE(TAG, "data send fail");
      }
    }
    vTaskDelay(1);
  }
}

String parseMac(const uint8_t *mac)
{
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(macStr);
}

void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  ConnectedTimeOut = CONNECT_TIMEOUT; // 接收到数据时重置超时
  // ESP_LOGI(TAG, "Recv data from : %s", parseMac(mac).c_str());
  if (PairRuning)
  {
    // 删除之前配对的从机
    esp_now_del_peer(slave.peer_addr);
    ConnectedTimeOut = CONNECT_TIMEOUT; // 接收到数据时重置超时
    slave.channel = CHANNEL;
    slave.encrypt = 0;
    memcpy(slave.peer_addr, mac, 6);
    esp_err_t addStatus = esp_now_add_peer(&slave);
    if (addStatus == ESP_OK || addStatus == ESP_ERR_ESPNOW_EXIST)
    {
      IsPaired = true;
      PairRuning = false;
      ESP_LOGI(TAG, "Recv data from : %s", parseMac(mac).c_str());
      ESP_LOGI(TAG, "Peer success ");
      ESP_LOGI(TAG, "Pair_Runing: %u, IsPaired : %u, ConnectedTimeOut: %u", PairRuning, IsPaired, ConnectedTimeOut);
    }
    else
    {
      ESP_LOGE(TAG, "Add peed Fail");
    }
  }
}

/* 检查连接同步状态，
一段时间内没有接收到回复信息，
或连续数据发送失败则认为连接已断开 */
void TaskConnectedWatch(void *pt)
{
  while (true)
  {
    if (IsPaired) // 连接成功时开始倒计时，超时设置状态为 “未连接”
    {
      ConnectedTimeOut--;
      if (ConnectedTimeOut <= 0)
      {
        IsPaired = false;
        ESP_LOGI(TAG, "Lost connection");
      }
    }
    vTaskDelay(1);
  }
}

/**
 * @brief 扫描&配对从机,检测  IsPaired 的状态，当未配对时将开始扫描并配对设备
 */
void TaskScanAndPeer(void *pt)
{

  while (true)
  {
    int16_t scanResults = 0;

    if (!IsPaired && !PairRuning) // 配对中或配对完成都不在扫描AP
    {
      ESP_LOGI(TAG, "Start scan");
      scanResults = WiFi.scanNetworks(0, 0, 0, 50, 1); // 扫描1通道
      if (scanResults != 0)
      {
        for (int i = 0; i < scanResults; i++)
        {
          SSID = WiFi.SSID(i);
          RSSI = WiFi.RSSI(i);
          BSSIDstr = WiFi.BSSIDstr(i);
          CHANNEL = WiFi.channel(i);

          if (SSID.indexOf("Slave") == 0)
          {
            ESP_LOGI(TAG, "Found a Slave : %u : SSID: %s, BSSID: [%s], RSSI:  (%u), Channel: %u", i + 1, SSID, BSSIDstr.c_str(), RSSI, CHANNEL);
            PairRuning = true; // 进入配对模式
            WiFi.scanDelete(); // 清除扫描信息
            break;             // 获取到第一个从机信息时结束for循环
          }
        }
      }
      else
      {
        ESP_LOGI(TAG, "No WiFi devices in AP Mode found,Try again");
        continue; // 跳过剩余 code 立即重新扫描
      }

      if (PairRuning)
      {
        ESP_LOGI(TAG, "To Peer");
        memset(&slave, 0, sizeof(slave)); // 清空对象

        int mac[6];
        sscanf(
            BSSIDstr.c_str(), "%x:%x:%x:%x:%x:%x",
            &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
        for (int v = 0; v < 6; ++v)
          slave.peer_addr[v] = (uint8_t)mac[v];
        slave.channel = CHANNEL;
        slave.encrypt = 0; // 不加密

        // 匹配信道
        esp_wifi_set_channel(slave.channel, WIFI_SECOND_CHAN_NONE);

        esp_err_t addStatus = esp_now_add_peer(&slave);
        // 配对成功 or 配对已存在即配对成功，否则重新循环并扫描配对AP
        if (addStatus == ESP_OK || addStatus == ESP_ERR_ESPNOW_EXIST)
        {
          uint8_t data = 0;
          ESP_LOGI(TAG, "Send peer message to %s", SSID.c_str());
          esp_now_send(slave.peer_addr, &data, sizeof(data));

          int conter = CONNECT_TIMEOUT;
          while (!IsPaired) // 等待从机发回 STA 模式 mac 地址
          {
            if (conter < 1)
            {
              ESP_LOGE(TAG, "Peer fail");
              PairRuning = false; // 等待回复信息超时退出配对模式
              break;
            }
            vTaskDelay(1);
            conter--;
          }
        }
      }
    }
    else
    {
      vTaskDelay(100);
    }
    vTaskDelay(1);
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
  Presend_data = presend_data;
  Send_gap_ms = send_gap_ms;
  ESP_LOGI(TAG, "init");
  // set wifi
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  // set esp now
  esp_now_init();
  esp_now_register_recv_cb(onDataRecv);
  ESP_LOGI(TAG, "STA MAC: %s, STA CHANNEL: %u", WiFi.macAddress().c_str(), WiFi.channel());
  ESP_LOGI(TAG, "SET All Task");
  // xTaskCreate(TaskConnectedWatch, "TaskConnectedWatch", 2048, NULL, 2, NULL);
  xTaskCreate(TaskScanAndPeer, "TaskScanAndPeer", 4096, NULL, 2, NULL);
  xTaskCreate(TaskSend, "TaskSend", 2048, NULL, 2, NULL);
  ESP_LOGI(TAG, "started");
}

/**
 * @brief 获取配对状态
 */
bool Radio::getPairStatus()
{
  return *isPaired;
}