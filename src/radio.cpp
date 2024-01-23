#include <esp_now.h>
#include <esp_wifi.h>
#include <radio.h>
#include <WiFi.h>

#define TAG "Radio"

String SSID;
int32_t RSSI;
String BSSIDstr;
int32_t CHANNEL;
uint8_t *key;
uint8_t *staMac;

#define PRINTSCANRESULTS 0
#define DELETEBEFOREPAIR 0

bool IsPaired = false;
esp_now_peer_info_t slave;

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
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    ESP_LOGI(TAG, "Recv data from : %s", macStr);
    // ESP_LOGI(TAG, "Recv data : %s", *incomingData);
    ESP_LOGI(TAG, "Peer success ");
    IsPaired = true;
  }
  else
  {
    ESP_LOGE(TAG, "Add peed Fail");
  }

  esp_now_unregister_recv_cb(); // 此回调在配对完成后删除
}

void TaskScanAndPeer(void *pt)
{
  memset(&slave, 0, sizeof(slave));
  Serial.println("Start scan");

  while (true)
  {
    bool findSlave = false;
    if (!IsPaired)
    {
      int16_t scanResults = WiFi.scanNetworks(0, 0, 0, 50, 1); // 扫描1通道
      if (scanResults == 0)
      {
        Serial.println("No WiFi devices in AP Mode found");
        Serial.println("Try again");
        continue; // 立即重新扫描
      }
      else
      {
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
            Serial.println("Found a Slave.");
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(SSID);
            Serial.print(" [");
            Serial.print(BSSIDstr);
            Serial.print("]");
            Serial.print(" (");
            Serial.print(RSSI);
            Serial.print(")");
            Serial.print("Channel :");
            Serial.print(CHANNEL);
            Serial.println("");

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
        Serial.println("to peer");
        esp_err_t addStatus = esp_now_add_peer(&slave);

        // 配对成功 or 配对已存在即配对成功，否则重新循环并扫描配对AP
        // TODO 向从机发送配对成功信息
        if (addStatus == ESP_OK || addStatus == ESP_ERR_ESPNOW_EXIST)
        {

          esp_now_send(slave.peer_addr, key, sizeof(key));
          Serial.print("Sennd - ");
          Serial.println(*key);

          esp_now_register_recv_cb(PeerDataRecv); // 此回调用于配对
          int conter = 0;
          while (!IsPaired) // 等待从机发回 STA 模式 mac 地址
          {
            if (conter > 5)
            {
              ESP_LOGE(TAG, "Peer fail");
              esp_now_unregister_recv_cb();
              break;
            }
            vTaskDelay(50);
            conter++;
          }

          // IsPaired = true;
        }
      }

      vTaskDelay(5);
    }
    else
    {
      // Serial.println("Paired");
      vTaskDelay(100);
    }
  }
}

void Radio::begin()
{
  isPaired = &IsPaired;
  vehcile = &slave;

  // 初始化时生成一个用于配对的随机钥匙
  // srand((unsigned)time(NULL));
  long v = random();
  key = (uint8_t *)&v;

  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  if (esp_now_init() == ESP_OK)
  {
    Serial.println("ESPNow Init Success");
  }
  else
  {
    Serial.println("ESPNow Init Failed...restarting...");
    ESP.restart();
  }
  Serial.print("STA MAC: ");
  Serial.println(WiFi.macAddress());
  Serial.print("STA CHANNEL ");
  Serial.println(WiFi.channel());
}

void Radio::startPairing()
{
  // 扫描&配对从机
  xTaskCreate(TaskScanAndPeer, "TaskScanAndPeer", 4096, NULL, 2, NULL);
}