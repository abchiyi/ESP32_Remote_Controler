#include <Arduino.h>
#include <esp_log.h>

// 无线依赖
#include <esp_now.h>
#include <esp_wifi.h>
// #include <radio.h>
#include <WiFi.h>

// 控制器依赖
#include <controller.h>

// 显示
#include <display.h>

#define TAG "Main ESP32 RC"

Radio radio;
Controller controller;

Display display;

esp_err_t
sendCB(uint8_t *peer_addr)
{
  return esp_now_send(peer_addr,
                      (uint8_t *)&controller.data,
                      sizeof(controller.data));
}
void setup()
{
  Serial.begin(115200);
  controller.begin();
  delay(1000); // 延迟启动无线连接
  radio.begin(sendCB, 10);
  display.begin(&radio);
}

void loop()
{
}