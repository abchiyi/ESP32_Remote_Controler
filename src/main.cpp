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

  /** 启动蓝牙控制器接入 **/
  if ((CONFIG.control_mode == MASTER) || (CONFIG.radio_mode == BT_CONTROLLER))
    Controller.begin();

  auto taskCrtpPacket = [](void *pt)
  {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = HZ2TICKS(80);
    static radio_packet_t rp;
    auto crtp = (CRTPPacket *)rp.data;

    static float ROLL;
    static float PITCH;
    static float YAW;
    static uint16_t THRUST;

    auto match_angl = [](uint8_t angl, int16_t joy)
    {
      auto step = angl / 2048.f;
      return joy * step;
    };

    Serial.println("");
    while (true)
    {
      xTaskDelayUntil(&xLastWakeTime, xFrequency);
      if (!Controller.is_connected())
        continue;
      ROLL = match_angl(40, Controller.getAnalogHat(joyLHori));
      PITCH = match_angl(255, Controller.getAnalogHat(joyLVert));
      YAW = match_angl(60, Controller.getAnalogHat(joyRHori));
      THRUST = (Controller.getAnalogHat(trigLT));
      crtp->port = CRTP_PORT_SETPOINT;
      crtp->channel = 0;

      memcpy(&crtp->data[0], &ROLL, sizeof(ROLL));
      memcpy(&crtp->data[4], &PITCH, sizeof(PITCH));
      memcpy(&crtp->data[8], &YAW, sizeof(YAW));
      memcpy(&crtp->data[12], &THRUST, sizeof(THRUST));

      // Serial.printf("\r ROLL: %.2f, PITCH: %.2f, YAW: %.2f, THRUST: %d                     ", ROLL, PITCH, YAW, THRUST);

      radio_send_packet(&rp);
    }
  };

  xTaskCreate(taskCrtpPacket, "taskCrtpPacket", 1024 * 3, NULL, TP_H, NULL);

  vTaskDelete(NULL); // 干掉 loopTask
}
void loop()
{
}
