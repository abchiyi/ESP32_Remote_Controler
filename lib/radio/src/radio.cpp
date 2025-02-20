
#include "radio.h"

#include <WiFi.h>
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include <vector>
#include <esp_wifi.h>
#include <algorithm>
#include <esp_err.h>
#include <cstring>
#include <esp_mac.h>
#include <tool.h>
#include <config.h>

#define TAG "Radio"

Radio RADIO;

QueueHandle_t Q_RECV_DATA = xQueueCreate(2, sizeof(radio_packet_t));

// 主任务
void TaskRadioMainLoop(void *pt)
{
  const static TickType_t xFrequency = pdMS_TO_TICKS(8);
  static TickType_t xLastWakeTime = xTaskGetTickCount();

  while (true)
  {
    RADIO.update_loop_metrics();

    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

/**
 * @brief 启动无线
 */
void Radio::begin()
{
  auto ret = xTaskCreatePinnedToCore(TaskRadioMainLoop, "TaskRadioMainLoop",
                                     2048, NULL, TP_H, NULL, 0);
  assert(ret == pdPASS);
  ESP_LOGI(TAG, "Radio started :)");
}
