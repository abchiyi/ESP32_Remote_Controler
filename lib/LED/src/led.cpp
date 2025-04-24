#include "led.h"
#include "FastLED.h"
#include "XBOX.h"
#include "config.h"

#define TAG "LED"

const uint8_t led_pin = 10;
const uint8_t led_count = 1;
CRGB leds[led_count];

#define BLACK CRGB::Black
#define WHITE CRGB::White
#define RED CRGB::Red
#define GREEN CRGB::Green
#define BLUE CRGB::Blue

void task_led_status(void *pvParameters)
{

    bool led_light_up = false;
    while (true)
    {

        if (!Controller.is_connected())
        {
            led_light_up = !led_light_up;
            leds[0] = led_light_up ? BLUE : BLACK;
            vTaskDelay(300);
        }
        else
        {
            leds[0] = GREEN;
            vTaskDelay(300);
        }

        FastLED.show();
    }
}

void init_led()
{
    FastLED.addLeds<WS2812B, led_pin, GRB>(leds, led_count);
    FastLED.setBrightness(255);
    FastLED.show();

    xTaskCreate(task_led_status, "task_led_status", 1024, NULL, TP_L, NULL);
}
