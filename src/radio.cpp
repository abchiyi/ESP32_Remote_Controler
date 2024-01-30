#include <esp_now.h>
#include <esp_wifi.h>
#include <radio.h>
#include <WiFi.h>

#define TAG "Radio"

String SSID;
int32_t RSSI;
String BSSIDstr;
int32_t CHANNEL;

esp_now_peer_info_t slave;

int CONNECT_TIMEOUT = 500;              // ms // 连接同步等待时间
int ConnectedTimeOut = CONNECT_TIMEOUT; // ms
const int MinSendGapMs = 8;
bool PairRuning = false;
bool IsPaired = false;
int Send_gap_ms = 0;

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

void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  ConnectedTimeOut = CONNECT_TIMEOUT; // 接收到数据时重置超时

  // ESP_LOGI(TAG, "Recv data from : %s", parseMac(mac).c_str());
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
      IsPaired = true;
    }
  }
}

void EspNowInit()
{
  static int counter = 0;
  // set esp now
  if (esp_now_init() == ESP_OK)
  {
    ESP_LOGI(TAG, "ESP NOW init success");
    esp_err_t refStatus = esp_now_register_recv_cb(onDataRecv);
    refStatus == ESP_OK
        ? ESP_LOGI(TAG, "Register recv cb success")
        : ESP_LOGE(TAG, "Register recv cb fail");
    counter = 0;
  }
  else
  {
    counter++;
    counter <= 5
        ? ESP_LOGE(TAG, "ESP NOW init fail, Try again...")
        : ESP_LOGE(TAG, "ESP NOW init fail, Maximum depth, restart...");
    delay(10);
    counter <= 5 ? EspNowInit() : ESP.restart();
  }
}

void ClearAllPair()
{
  if (esp_now_deinit() == ESP_OK)
  {
    ESP_LOGI(TAG, "Clear All paired slave");
    EspNowInit();
  }
}

void TaskSend(void *pt)
{
  while (true)
  {
    if (IsPaired)
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
        ESP_LOGI(TAG, "Lost connection");
        IsPaired = false;
        ClearAllPair();
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

        ESP_LOGI(TAG, "Pair to %s", SSID);
        if (Pair(slave))
        {
          uint8_t data = 0;
          ESP_LOGI(TAG, "Send peer message to %s", SSID.c_str());
          esp_now_send(slave.peer_addr, &data, sizeof(data));

          // 等待从机发回 STA 模式 mac 地址
          int conter = 100; // ms
          while (!IsPaired)
          {
            if (conter < 1)
            {
              ESP_LOGE(TAG, "Recv data time out");
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
 * @param cb_fn 发送回调
 * @param send_gap_ms 数据同步间隔 /ms,最小值为 MinSendGapMs 定义的值
 */
void Radio::begin(send_cb_t cb_fn, int send_gap_ms)
{
  isPaired = &IsPaired;
  vehcile = &slave;
  SENDCB = cb_fn;
  Send_gap_ms = send_gap_ms <= MinSendGapMs ? MinSendGapMs : send_gap_ms;
  CONNECT_TIMEOUT = Send_gap_ms + 100; // 配置接收等待时间

  ESP_LOGI(TAG, "init");
  // set wifi
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  ESP_LOGI(TAG, "STA MAC: %s, STA CHANNEL: %u", WiFi.macAddress().c_str(), WiFi.channel());

  // set esp now
  EspNowInit();

  ESP_LOGI(TAG, "SET All Task");
  xTaskCreate(TaskConnectedWatch, "TaskConnectedWatch", 2048, NULL, 2, NULL);
  xTaskCreate(TaskScanAndPeer, "TaskScanAndPeer", 4096, NULL, 2, NULL);
  xTaskCreate(TaskSend, "TaskSend", 2048, NULL, 2, NULL);
}

/**
 * @brief 获取配对状态
 */
bool Radio::getPairStatus()
{
  return *isPaired;
}