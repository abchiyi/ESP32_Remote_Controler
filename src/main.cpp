#include <Arduino.h>
#include <esp_log.h>
// 无线依赖
#include "radio.h"
#include "tool.h"
#include "config.h"

#include "dataTransmit.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// 控制器依赖
#include "XBOX.h"
#include <pins_arduino.h>

// LED
#include "led.h"
#include "bat.h"
#include "web_console.h"
#include "array"

#define TAG "Main ESP32 RC"

void setup()
{
  Serial.begin(115200);

  CONFIG.begin(); // 初始化配置

  // ** 电池初始化 **
  init_bat();
  vTaskDelay(300); // 等待电池电压稳定

  // ** LED初始化 **
  init_led();

  // ** Web控制台初始化 **
  init_web_console(); // 启动Web控制台
  vTaskDelay(100);

  // ** 无线初始化 **/
  init_transmit(); // 初始化无线传输

  vTaskDelete(NULL); // 干掉 loopTask
}
void loop()
{
}

/**
 * @brief 处理接收到的设定点数据。
 */
void recv_setpoint(radio_packet_t *packet)
{
  auto cp = (CRTPPacket *)packet->data;
  auto sp = (packet_setpoint_t *)cp->data;
}

/**
 * @brief 将需要发送到主机的数据填充到 packet 中
 */
void send_on_slave(radio_packet_t *packet)
{
}
