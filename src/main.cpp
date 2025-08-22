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

// MOTOR
#include "Rz_series.h"

// SERVO
#include "ESP32Servo.h"
Servo servo;

#define FI 1
#define BI 0
#define PIN_SERVO 3
#define YAW_MAX 100

#define TAG "Main ESP32 RC"
RZ_HBridgeDriver motor(FI, BI);

static uint8_t backward = 0;
static uint8_t forward = 0;
static uint8_t yaw = 90;

void vehicle_control_task(void *pvParameters)
{

  while (true)
  {
    servo.write(yaw);
    if (forward)
      while (true)
      {
        servo.write(yaw);

        if (backward)
          motor.stop();
        else
          motor.forward(forward);

        if (!forward && !backward)
          break;

        vTaskDelay(50);
      }

    if (backward)
      while (true)
      {
        servo.write(yaw);
        if (forward)
          motor.stop();
        else
          motor.backward(backward);

        if (!forward && !backward)
          break;

        vTaskDelay(50);
      }

    vTaskDelay(50);
  }
}

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

  // ** 初始化 舵机 **/
  ESP32PWM::allocateTimer(1);
  servo.setPeriodHertz(50);
  servo.attach(PIN_SERVO, 1000, 2000);

  xTaskCreate(vehicle_control_task, "vehicle_control_task", 1024 * 3, NULL, TP_N, NULL);

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

  forward = map(sp->THRUST, 0, UINT16_MAX, 0, 255);
  backward = map(sp->REVERSE, 0, UINT16_MAX, 0, 255);
  yaw = map(sp->YAW, 0, 180, 35, 145);
}

/**
 * @brief 将需要发送到主机的数据填充到 packet 中
 */
void send_on_slave(radio_packet_t *packet)
{
}
