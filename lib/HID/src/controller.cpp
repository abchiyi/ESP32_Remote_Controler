#include <controller.h>
#include "FreeRTOS.h"
#include "config.h"
#include "tool.h"
#include "esp_log.h"

#define TAG "Controller"

#include "Arduino.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"

#include "esp_hidh.h"
#include "esp_hid_gap.h"
#include "esp_timer.h"

#define XBOX_CONTROLLER_INDEX_BUTTONS_DIR 12
#define XBOX_CONTROLLER_INDEX_BUTTONS_MAIN 13
#define XBOX_CONTROLLER_INDEX_BUTTONS_CENTER 14
#define XBOX_CONTROLLER_INDEX_BUTTONS_SHARE 15

#define SCAN_DURATION_SECONDS 5

static auto Q_RECV = xQueueCreate(10, sizeof(xbox_control_data));

CONTROLLER Controller;

void hid_task(void *pvParameters)
{
  size_t results_len = 0;
  esp_hid_scan_result_t *results = NULL;
  ESP_LOGI(TAG, "SCAN...");
  // start scan for HID devices
  esp_hid_scan(SCAN_DURATION_SECONDS, &results_len, &results);
  ESP_LOGI(TAG, "SCAN: %u results", results_len);
  if (results_len)
  {
    esp_hid_scan_result_t *r = results;
    esp_hid_scan_result_t *cr = NULL;
    while (r)
    {
      printf("  %s: " ESP_BD_ADDR_STR ", ", (r->transport == ESP_HID_TRANSPORT_BLE) ? "BLE" : "BT ", ESP_BD_ADDR_HEX(r->bda));
      printf("RSSI: %d, ", r->rssi);
      printf("USAGE: %s, ", esp_hid_usage_str(r->usage));

      if (r->transport == ESP_HID_TRANSPORT_BLE)
      {
        cr = r;
        printf("APPEARANCE: 0x%04x, ", r->ble.appearance);
        printf("ADDR_TYPE: '%s', ", ble_addr_type_str(r->ble.addr_type));
      }

      printf("NAME: %s ", r->name ? r->name : "");
      printf("\n");
      r = r->next;
    }
    if (cr)
    {
      // open the last result
      esp_hidh_dev_open(cr->bda, cr->transport, cr->ble.addr_type);
    }
    // free the results
    esp_hid_scan_results_free(results);
  }
  ESP_LOGW(TAG, "SCAN: done, Task ended");
  vTaskDelete(NULL);
}

void hidh_callback(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
  esp_hidh_event_t event = (esp_hidh_event_t)id;
  esp_hidh_event_data_t *param = (esp_hidh_event_data_t *)event_data;

  switch (event)
  {
  case ESP_HIDH_OPEN_EVENT:
  {
    if (param->open.status == ESP_OK)
    {
      const uint8_t *bda = esp_hidh_dev_bda_get(param->open.dev);
      ESP_LOGI(TAG, ESP_BD_ADDR_STR " OPEN: %s", ESP_BD_ADDR_HEX(bda), esp_hidh_dev_name_get(param->open.dev));
      esp_hidh_dev_dump(param->open.dev, stdout);
    }
    else
    {
      ESP_LOGE(TAG, " OPEN failed!");
    }
    break;
  }
  case ESP_HIDH_BATTERY_EVENT:
  {
    const uint8_t *bda = esp_hidh_dev_bda_get(param->battery.dev);
    ESP_LOGI(TAG, ESP_BD_ADDR_STR " BATTERY: %d%%", ESP_BD_ADDR_HEX(bda), param->battery.level);
    break;
  }
  case ESP_HIDH_INPUT_EVENT:
  {

    // const uint8_t *bda = esp_hidh_dev_bda_get(param->input.dev);
    // ESP_LOGI(TAG, ESP_BD_ADDR_STR " INPUT: %8s, MAP: %2u, ID: %3u, Len: %d, Data:", ESP_BD_ADDR_HEX(bda), esp_hid_usage_str(param->input.usage), param->input.map_index, param->input.report_id, param->input.length);
    // ESP_LOG_BUFFER_HEX(TAG, param->input.data, param->input.length);
    // ESP_LOGI(TAG, "id:%3u", param->input.report_id);

    uint8_t btnBits;
    /*
    btnA = btnBits & 0b00000001;
    btnB = btnBits & 0b00000010;
    btnX = btnBits & 0b00001000;
    btnY = btnBits & 0b00010000;
    btnLB = btnBits & 0b01000000;
    btnRB = btnBits & 0b10000000;
    */
    btnBits = param->input.data[XBOX_CONTROLLER_INDEX_BUTTONS_MAIN];
    Controller.button_bits[btnA] = (btnBits & 0b00000001);
    Controller.button_bits[btnB] = (btnBits & 0b00000010);
    Controller.button_bits[btnX] = (btnBits & 0b00001000);
    Controller.button_bits[btnY] = (btnBits & 0b00010000);
    Controller.button_bits[btnLB] = (btnBits & 0b01000000);
    Controller.button_bits[btnRB] = (btnBits & 0b10000000);

    /*
    btnSelect = btnBits & 0b00000100;
    btnStart = btnBits & 0b00001000;
    btnXbox = btnBits & 0b00010000;
    btnLS = btnBits & 0b00100000;
    btnRS = btnBits & 0b01000000;
    */

    btnBits = param->input.data[XBOX_CONTROLLER_INDEX_BUTTONS_CENTER];
    Controller.button_bits[btnSelect] = (btnBits & 0b00000100);
    Controller.button_bits[btnStart] = (btnBits & 0b00001000);
    Controller.button_bits[btnXbox] = (btnBits & 0b00010000);
    Controller.button_bits[btnLS] = (btnBits & 0b00100000);
    Controller.button_bits[btnRS] = (btnBits & 0b01000000);

    /*
    btnShare = btnBits & 0b00000001;
    */
    btnBits = param->input.data[XBOX_CONTROLLER_INDEX_BUTTONS_SHARE];
    Controller.button_bits[btnShare] = (btnBits & 0b00000001);

    btnBits = param->input.data[XBOX_CONTROLLER_INDEX_BUTTONS_DIR];
    auto dirUP = btnBits == 1 || btnBits == 2 || btnBits == 8;
    auto dirRight = 2 <= btnBits && btnBits <= 4;
    auto dirDown = 4 <= btnBits && btnBits <= 6;
    auto dirLeft = 6 <= btnBits && btnBits <= 8;

    Controller.button_bits[btnDirUp] = dirUP;
    Controller.button_bits[btnDirRight] = dirRight;
    Controller.button_bits[btnDirDown] = dirDown;
    Controller.button_bits[btnDirLeft] = dirLeft;

    /*
     * joyLHori 0
     * joyLVori 2
     * joyRHori 4
     * joyRVori 6
     * trigLT   8
     * trigRT   10
     */
    auto read_analog = [&](uint8_t index)
    {
      return (uint16_t)param->input.data[index] |
             ((uint16_t)param->input.data[index + 1] << 8);
    };

    Controller.analog_hat[joyLHori] = analogHatFilter(read_analog(0));
    Controller.analog_hat[joyLVert] = analogHatFilter(read_analog(2));
    Controller.analog_hat[joyRHori] = analogHatFilter(read_analog(4));
    Controller.analog_hat[joyRVert] = analogHatFilter(read_analog(6));
    Controller.analog_hat[trigLT] = read_analog(8);
    Controller.analog_hat[trigRT] = read_analog(10);

    break;
  }
  case ESP_HIDH_FEATURE_EVENT:
  {
    const uint8_t *bda = esp_hidh_dev_bda_get(param->feature.dev);
    ESP_LOGI(TAG, ESP_BD_ADDR_STR " FEATURE: %8s, MAP: %2u, ID: %3u, Len: %d", ESP_BD_ADDR_HEX(bda),
             esp_hid_usage_str(param->feature.usage), param->feature.map_index, param->feature.report_id,
             param->feature.length);
    ESP_LOG_BUFFER_HEX(TAG, param->feature.data, param->feature.length);
    break;
  }
  case ESP_HIDH_CLOSE_EVENT:
  {
    const uint8_t *bda = esp_hidh_dev_bda_get(param->close.dev);
    ESP_LOGI(TAG, ESP_BD_ADDR_STR " CLOSE: %s", ESP_BD_ADDR_HEX(bda), esp_hidh_dev_name_get(param->close.dev));

    xTaskCreate(&hid_task, "hid_task", 6 * 1024, NULL, 2, NULL);

    break;
  }
  default:
    ESP_LOGI(TAG, "EVENT: %d", event);
    break;
  }
}

void bt_controller_init()
{
  esp_err_t ret;

  ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  ESP_LOGI(TAG, "BT controller init");

  ESP_ERROR_CHECK(esp_hid_gap_init(HIDH_BLE_MODE));
  ESP_ERROR_CHECK(esp_ble_gattc_register_callback(esp_hidh_gattc_event_handler));

  esp_hidh_config_t config = {
      .callback = hidh_callback,
      .event_stack_size = 4096,
      .callback_arg = NULL,
  };
  ESP_ERROR_CHECK(esp_hidh_init(&config));

  xTaskCreate(&hid_task, "hid_task", 6 * 1024, NULL, 2, NULL);
};

// 启动xbox控制器
void CONTROLLER::begin()
{
  bt_controller_init();
}

bool CONTROLLER::getButtonPress(XBOX_BUTTON btn)
{
  return this->button_bits[btn];
}

int16_t CONTROLLER::getAnalogHat(XBOX_ANALOG_HAT hat)
{
  return this->analog_hat[hat];
}
