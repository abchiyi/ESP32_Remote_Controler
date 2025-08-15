
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

#define TAG "Radio"

QueueHandle_t Q_SEND_PACK = xQueueCreate(10, sizeof(radio_packet_t));

radio_cb_fn func_Array[16] = {nullptr};

radio_link_operation_t noLink{
    .recv = [](radio_packet_t *rp)
    { return ESP_FAIL; },

    .send = [](radio_packet_t *rp)
    { return ESP_FAIL; },

    .is_connected = []()
    { return false; },

    .start = []()
    { return ESP_FAIL; },

    .rest = []()
    { return ESP_FAIL; },
};

radio_link_operation_t *RLOP = &noLink;

IRAM_ATTR void task_radio_recv(void *pvParameters)
{
  static radio_packet_t rp;

  while (true)
  {
    if (RLOP->recv(&rp) == ESP_OK)
    {
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
      RLOP->send(&rp);
  }
}

void init_radio()
{
  radio_link_operation_t *rlop = nullptr;
  /*** 初始化无线通讯模式 ***/
  switch (CONFIG.radio_mode)
  {
  case ESP_NOW:
    rlop = WiFi_Esp_Now();
    ESP_LOGI(TAG, "Radio mode: ESP_NOW");
    break;

  default:
    ESP_LOGE(TAG, "Unsupported radio mode: %d", CONFIG.radio_mode);
    esp_system_abort("Radio mode is not supported");
    break;
  }

  if (rlop == nullptr)
  {
    ESP_LOGE(TAG, "Radio link operation is null");
    return;
  }

  ESP_ERROR_CHECK(rlop->start());
  RLOP = rlop;

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

bool radio_link_is_connected()
{
  return RLOP->is_connected();
}

esp_err_t radio_send_packet(radio_packet_t *rp)
{
  rp->checksum = calculate_cksum(rp->data, sizeof(rp->data));
  return RLOP->send(rp);
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
  if (RLOP->is_connected)
    return RLOP->is_connected();
  return false;
}
