#include <view/mainPage.h>
#include <view/menu.h>
#include <controller.h>
#include <radio.h>
#include <car.h>
#include <view/setting_devces.h>
#include "view/about.h"
#include "tool.h"

#define slider_width 6
#define slider_length 45

#define TAG "Main page"

float VBUS_V;
float CURREN_A;
float POWER_W;

void read_power_info(void *radio_data)
{
  auto data = *(radio_data_t *)radio_data;

  VBUS_V = combineFloat(data.channel[0], data.channel[1]);
  CURREN_A = combineFloat(data.channel[2], data.channel[3]);
  POWER_W = combineFloat(data.channel[4], data.channel[5]);
};

class MainPage : public BasePage
{

public:
  void create()
  {
    this->name = "Main page";
    ESP_LOGI(this->name, "page create");
    gui->add_event_listener(KEY_MENU, [&]()
                            { gui->page_in_to(create_page_menu); });
  };

  void before()
  {
    ESP_LOGI(TAG, "init page");
    u8g2->setFont(LIST_FONT);
    u8g2->setDrawColor(2);
  }

  void render_channel_view()
  {
    // TODO 通道数据可配置，而非固定
    float joy_l_x = Controller.joyLHori / 2048.0;
    float joy_l_y = Controller.joyLVert / 2048.0;

    float joy_r_x = Controller.joyRHori / 2048.0;
    float joy_r_y = Controller.joyRVert / 2048.0;

    // slider 1
    this->draw_slider_y(joy_l_y,
                        0,
                        gui->DISPLAY_HEIGHT - slider_length - slider_width - 4,
                        slider_width,
                        slider_length,
                        true);

    // slider 2
    this->draw_slider_y(joy_r_y,
                        gui->DISPLAY_WIDTH - slider_width,
                        gui->DISPLAY_HEIGHT - slider_length - slider_width - 4,
                        slider_width,
                        slider_length,
                        true);

    // slider 3
    this->draw_slider_x(joy_l_x,
                        0,
                        gui->DISPLAY_HEIGHT - slider_width,
                        slider_width,
                        slider_length,
                        true);

    // slider 4
    this->draw_slider_x(joy_r_x,
                        gui->DISPLAY_WIDTH - slider_length,
                        gui->DISPLAY_HEIGHT - slider_width,
                        slider_width,
                        slider_length,
                        true);
  };

  void render()
  {
    static TickType_t last_run;

    auto tick = xTaskGetTickCount();
    u8g2->setCursor(10, 10);
    u8g2->printf("BAT %.2fV", VBUS_V);
    u8g2->setCursor(10, 20);
    u8g2->printf("CUR %.2fA ", CURREN_A);
    u8g2->setCursor(10, 30);
    u8g2->printf("POWER %.2fW ", POWER_W);
    u8g2->setCursor(10, 40);
    u8g2->printf("FQ %d ", RADIO.Frequency);
    u8g2->setCursor(10, 50);
    u8g2->printf("RSSI %d ", RADIO.RSSI);
    last_run = tick;

    this->render_channel_view();
    // 连接状态
    u8g2->setCursor((gui->DISPLAY_WIDTH - 6 * 5) / 2, gui->DISPLAY_HEIGHT);
    u8g2->printf("< %s >", RADIO.status == RADIO_CONNECTED
                               ? "="
                               : "x");
  };
};

BasePage *create_page_main()
{
  return new MainPage;
}
