
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <esp_now.h>
#include <esp_wifi.h>
#include <radio.h>
#include <WiFi.h>

#include <esp_log.h>

// // callback when data is sent from Master to Slave
// void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
// {
//   char macStr[18];
//   snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
//            mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
//   Serial.print("Last Packet Sent to: ");
//   Serial.println(macStr);
//   Serial.print("Last Packet Send Status: ");
//   Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
//   Serial.print("This Mac :");
//   Serial.println(WiFi.macAddress());
// }

// void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
// {
//   static bool rr = false;
//   if (!rr)
//   {
//     rr = !rr;
//     deletePeer();
//     slave.channel = CHANNEL;
//     slave.encrypt = 0;
//     memcpy(slave.peer_addr, mac, 6);
//     esp_now_add_peer(&slave);
//   }

//   char macStr[18];

//   snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
//            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
//   Serial.print("Recv data from :");
//   Serial.println(macStr);
// }

Radio radio;

void setup()
{
  Serial.begin(115200);
  ESP_LOGI("TAG", "Total APs scanned = %u", 123);
  radio.begin();
  radio.startPairing();

  // esp_now_register_send_cb(OnDataSent);
  // esp_now_register_recv_cb(OnDataRecv);
}

int num = 0;
void loop()
{
  if (*radio.isPaired)
  {
    esp_now_send(radio.vehcile->peer_addr, (uint8_t *)&num, sizeof(&num));
    delay(100);
  }
  // bool isPaired = manageSlave();

  // if (!isPaired)
  //   ScanForSlave();

  // if (slave.channel == CHANNEL)
  // {
  //   if (isPaired)
  //   {
  //     sendData();
  //   }
  //   else
  //   {
  //     Serial.println("Slave pair failed!");
  //   }
  // }
  // else
  // {
  //   // No slave found to process
  // }
  // delay(1000);
}