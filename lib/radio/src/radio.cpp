
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
#include "wifi_link.h"
#include "bt_controller_link.h"

#define TAG "Radio"

QueueHandle_t Q_SEND_PACK = xQueueCreate(10, sizeof(radio_packet_t));

radio_cb_fn func_Array[16] = {nullptr};

static auto NoLink = new RadioLink();
static RadioLink *radio_link = NoLink;

IRAM_ATTR void
task_radio_recv(void *pvParameters)
{
  static radio_packet_t rp;

  while (true)
  {
    if (radio_link->recv(&rp) == ESP_OK)
    {
      // 如果有回调函数，调用它
      auto fn = func_Array[((CRTPPacket *)rp.data)->port];
      if (fn != nullptr)
        fn(&rp);
    }
  }
}

IRAM_ATTR void task_radio_send(void *pvParameters)
{
  static radio_packet_t rp;
  while (true)
  {
    if (xQueueReceive(Q_SEND_PACK, &rp, portMAX_DELAY) == pdTRUE)
      radio_link->send(&rp);
  }
}

void init_radio()
{
  ESP_LOGI(TAG, "Radio init ...");
  /*** 初始化无线通讯模式 ***/
  switch (CONFIG.radio_mode)
  {
  case ESP_NOW:
    radio_link = WiFi_Esp_Now();
    ESP_LOGI(TAG, "Radio mode: ESP_NOW");
    break;

  case BT_CONTROLLER:
    radio_link = bt_Controller_link();
    break;

  default:
    ESP_LOGE(TAG, "Unsupported radio mode: %d", CONFIG.radio_mode);
    esp_system_abort("Radio mode is not supported");
    break;
  }

  ESP_ERROR_CHECK(radio_link->start());

  if (radio_link == nullptr)
  {
    ESP_LOGE(TAG, "Radio link operation is null");
    configASSERT(radio_link != nullptr);
  }

  auto ret = xTaskCreate(task_radio_recv, "TaskRadioRecv",
                         2048, NULL, TP_H, NULL);
  if (ret != pdPASS)
  {
    ESP_LOGE(TAG, "Create task_radio_recv failed");
    return;
  }

  ret = xTaskCreate(task_radio_send, "TaskRadioSend",
                    2048, NULL, TP_H, NULL);
  if (ret != pdPASS)
  {
    ESP_LOGE(TAG, "Create task_radio_send failed");
    return;
  }

  ESP_LOGI(TAG, "Radio started :)");
}

esp_err_t radio_send_packet(radio_packet_t *rp)
{
  rp->checksum = calculate_cksum(rp->data, sizeof(rp->data));
  return radio_link->send(rp);
}

/**
 * @brief 设置指定端口的回调函数。
 *
 * 此函数将回调函数 `fn` 绑定到指定的 `CRTPPort` 端口。如果该端口已经有回调函数，则返回失败。
 *
 * @param port 要设置回调的端口。
 * @param fn 要绑定的回调函数。
 * @return esp_err_t 返回操作结果，成功时返回 ESP_OK，失败时返回 ESP_FAIL。
 */
esp_err_t radio_set_port_callback(CRTPPort port, radio_cb_fn fn)
{
  if (!func_Array[port])
  {
    func_Array[port] = fn;
    return ESP_OK;
  }
  return ESP_FAIL;
}

bool radio_is_connected()
{
  return radio_link->is_connected();
}
