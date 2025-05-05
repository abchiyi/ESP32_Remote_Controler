#include "led.h"
#include "FastLED.h"
#include "XBOX.h"
#include "config.h"
#include "radio.h"
#include "bat.h"
#include "tool.h"

#define TAG "LED"

const uint8_t led_pin = 10;
const uint8_t led_count = 11;
CRGB leds[led_count];

#define BLACK CHSV(0, 0, 0)
#define GREEN CHSV(89, 200, 255)
#define BLUE CHSV(137, 189, 245)
static CRGB COLOR_BY_MAC;

#define LED_REFRESH_RATE 60 // 最高刷新频率/Hz

enum led_light_show_mode
{
    RAINBOW = 1,        // 波浪彩虹
    SOLID_COLOR,        // 定义的单色
    SOLID_COLOR_BY_MAC, // 根据本机MAC后三位地址定义的单色
    BATTERY             // 电池电量
};

static bool controller_is_connected = false; // 控制器连接状态
static bool controller_int;                  // 控制器连接指示
static bool radio_connected = false;         // 无线连接状态
static bool battery_low = false;             // 低电量指示

void fill_leds(CRGB color)
{
    fill_solid(leds, led_count, color);
}

void led_show_battery()
{
    auto battery_level = get_battery_percentage();
    if (battery_level < 50)
        battery_low = true;
    // ESP_LOGI(TAG, "battery_level: %d", battery_level);
}

void led_light_show()
{
    for (size_t hue = 255; hue > 0; hue--)
    {
        for (size_t i = 0; i < led_count; i++)
            leds[i] = CHSV(hue, 255, 255);
        FastLED.show();
        vTaskDelay(10);
    }
}

void led_show_bt_status()
{
    static bool led_light_up = false;
    // 蓝牙连接指示
    // 等待连接闪烁
    if (!controller_is_connected)
    {
        led_light_up = !led_light_up;
        fill_solid(leds, led_count, led_light_up ? BLUE : BLACK);
        FastLED.show();
        vTaskDelay(300);
    }
    // 连接成功常亮
    else if (controller_is_connected && controller_int)
    {
        controller_int = false; // 重置连接指示
        fill_leds(BLUE);
        FastLED.show();
        vTaskDelay(2000);
    }
}

void led_show_radio_channel()
{
    static bool led_light_up = false;
    if (battery_low)
    {
        for (size_t i = 0; i < 3; i++)
        {
            fill_leds(CRGB::Red);
            FastLED.show();
            vTaskDelay(100);
            fill_leds(CRGB::Black);
            FastLED.show();
            vTaskDelay(100);
        }

        vTaskDelay(500);
        return;
    }

    if (!radio_connected)
    {
        led_light_up = !led_light_up;
        fill_leds(led_light_up ? COLOR_BY_MAC : CRGB::Black);
        FastLED.show();
        vTaskDelay(300);
    }
    else
    {
        fill_leds(COLOR_BY_MAC);
        FastLED.show();
    }
}

void task_led_status(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    TickType_t xFrequency = HZ2TICKS(LED_REFRESH_RATE);

    while (true)
    {
        vTaskDelayUntil(&xLastWakeTime, LED_REFRESH_RATE);
        radio_connected = radio_is_connected();
        led_show_battery();

        if (!controller_is_connected) // TODO 当不启蓝牙功能时，不显示蓝牙状态
        {
            led_show_bt_status();
            continue;
        }
        else if (!radio_connected || battery_low)
        {
            led_show_radio_channel();
            continue;
        }
    }
}

void init_led()
{
    FastLED.addLeds<WS2812B, led_pin, GRB>(leds, led_count)
        .setCorrection(TypicalLEDStrip)
        .setDither(BINARY_DITHER);
    FastLED.setBrightness(255);

    Controller.setCallBack(XBOX_ON_CONNECTED, []()
                           { controller_is_connected = true;
                            controller_int = true; });

    Controller.setCallBack(XBOX_ON_DISCONNECTED, []()
                           { controller_is_connected = false; });

    // Get the last three bytes of the MAC address
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    uint8_t r = mac[3];
    uint8_t g = mac[4];
    uint8_t b = mac[5];

    COLOR_BY_MAC = CRGB(r, g, b);

    xTaskCreate(task_led_status, "task_led_status", 2048, NULL, TP_L, NULL);
}
