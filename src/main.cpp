#include <Arduino.h>
#include <esp_log.h>
// 无线依赖
#include "radio.h"
#include "tool.h"
#include "config.h"

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

typedef struct
{
  union
  {
    struct
    {
      float ROLL;
      float PITCH;
      float YAW;
      uint16_t THRUST;
      bool breaker;
      // 回传数据
      int16_t rssi;    // 接收信号强度指示
      int16_t voltage; // 电池电压
    };

    uint8_t raw[CRTP_MAX_DATA_SIZE];
  };

} __attribute((packed)) control_data_t;

void setup()
{
  Serial.begin(115200);

  CONFIG.begin(); // 初始化配置

  // ** 电池初始化 **
  init_bat();
  vTaskDelay(300); // 等待电池电压稳定

  // ** LED初始化 **
  init_led();

  // ** 无线初始化 **/
  init_radio();

  // ** Web控制台初始化 **
  init_web_console(); // 启动Web控制台

  // ** 特定条件初始化蓝牙以连接无线控制器 **

  if (CONFIG.control_mode == MASTER || CONFIG.radio_mode == BT_CONTROLLER)
    Controller.begin();

  vTaskDelete(NULL); // 干掉 loopTask
}
void loop()
{
}
