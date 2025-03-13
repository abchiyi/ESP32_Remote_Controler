#include <view/mainPage.h>
#include <view/menu.h>
#include <controller.h>
#include <radio.h>
#include <view/setting_devces.h>
#include "view/about.h"
#include "tool.h"
#include "WiFi.h"
#include "radio.h"
#include "controller.h"

#define slider_width 6
#define slider_length 45

#define TAG "Main page"

float VBUS_V;
float CURREN_A;
float POWER_W;

void read_power_info(void *radio_data) {
  // auto data = *(radio_packet_t *)radio_data;

  // VBUS_V = combineFloat(data.channel[0], data.channel[1]);
  // CURREN_A = combineFloat(data.channel[2], data.channel[3]);
  // POWER_W = combineFloat(data.channel[4], data.channel[5]);
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

  void draw_ico_connected(uint8_t x_start)
  {
    static bool tr = true;
    static int counter = 0;

    auto ric = radio_is_connected();

    u8g2->setFont(u8g2_font_siji_t_6x10);
    if (ric || tr)
      u8g2->drawGlyph(x_start, 10, 0x0e19c);

    if (ric)
      return;

    if (tr)
    {
      if (counter >= 20)
        tr = false;
      else
        counter++;
    }
    else
    {
      if (counter == 0)
        tr = true;
      else
        counter--;
    }
  }
  void draw_ico_usb(uint8_t x_start)
  {
    static bool trc = true;
    static int counter = 0;

    auto ric = Xbox.XboxOneConnected;

    u8g2->setFont(u8g2_font_siji_t_6x10);
    if (ric || trc)
      u8g2->drawGlyph(x_start, 10, 0x0e00c);

    if (ric)
      return;

    if (trc)
    {
      if (counter >= 20)
        trc = false;
      else
        counter++;
    }
    else
    {
      if (counter == 0)
        trc = true;
      else
        counter--;
    }
  }

  void render()
  {
    u8g2->setFont(u8g2_font_siji_t_6x10);
    u8g2->drawGlyph(0, 10, 0x0e21a);
    u8g2->drawGlyph(12, 10, 0x0e200);

    this->draw_ico_connected(24);
    this->draw_ico_usb(36);

    u8g2->setFont(u8g2_font_12x6LED_tf);
    u8g2->setCursor(0, 32);
    u8g2->println("READY");
    // u8g2->println("LOCK");

    u8g2->setCursor(80, 12);
    u8g2->println("FF:EA:12");

    u8g2->setFont(u8g2_font_9x6LED_tf);
    auto w = 128 - u8g2->getStrWidth("12.75V");
    u8g2->setCursor(w, 27);

    u8g2->print("12.75V");
  };
};

BasePage *create_page_main()
{
  return new MainPage;
}
