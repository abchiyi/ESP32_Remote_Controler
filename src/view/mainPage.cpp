#include <view/mainPage.h>
#include <view/menu.h>
#include <controller.h>
#include <radio.h>
#include <view/setting_devces.h>
#include "view/about.h"
#include "tool.h"
#include "radio.h"
#include "controller.h"

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
      u8g2->drawGlyph(x_start, 9, 0x0e19c);

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

    // auto ric = Xbox.XboxOneConnected;
    auto ric = 0;

    u8g2->setFont(u8g2_font_siji_t_6x10);
    if (ric || trc)
      u8g2->drawGlyph(x_start, 9, 0x0e00c);

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

  void render_control_data()
  {
    static const uint8_t display_width = gui->DISPLAY_WIDTH;

    static auto title = "R P Y T";
    static const auto title_width = u8g2->getStrWidth(title);   // title宽度
    static const uint8_t title_height = 9;                      // title高度
    static const uint8_t x_start = display_width - title_width; // x轴起始点

    static const uint8_t x_Roll = x_start;      // Roll x轴起始点
    static const uint8_t x_Pitch = x_Roll + 12; // Pitch x轴起始点
    static const uint8_t x_Yaw = x_Pitch + 12;  // Yaw x轴起始点
    static const uint8_t x_Thrust = x_Yaw + 12; // Thrust x轴起始点

    // draw title
    u8g2->setFont(u8g2_font_9x6LED_tf);
    u8g2->setCursor(x_start, title_height);
    u8g2->println(title);

    static const uint8_t pos_y = 11;    // 滑动条y轴起始点
    static const uint8_t s_width = 6;   // 滑动条宽度
    static const uint8_t s_length = 21; // 滑动条长度

    // draw control data
    float joy_l_y = Controller.getAnalogHat(joyLVert) / 2048.0;

    auto r_draw_slider_y = [=](float data, uint8_t pos_x)
    {
      this->draw_slider_y(data, pos_x, pos_y, s_width, s_length, true);
    };

    r_draw_slider_y(joy_l_y, x_Roll);   // Roll
    r_draw_slider_y(joy_l_y, x_Pitch);  // Pitch
    r_draw_slider_y(joy_l_y, x_Yaw);    // Yaw
    r_draw_slider_y(joy_l_y, x_Thrust); // Thrust
  }

  void render_ico()
  {
    static const uint8_t x_start = 0;
    static const uint8_t y_end = 9;

    u8g2->setFont(u8g2_font_siji_t_6x10);

    // battery
    u8g2->drawGlyph(x_start, y_end, 0x0e1fd);
    // u8g2->drawGlyph(x_start + 15, y_end, 0x0e25d);
    u8g2->drawGlyph(x_start + 15, y_end, 0x0e12b);
    u8g2->drawGlyph(x_start + 25, y_end, 0x0e261);
  };

  void render()
  {

    this->render_ico();

    u8g2->setFont(u8g2_font_9x6LED_tf);
    u8g2->setCursor(42, 32);
    u8g2->println("12.5V");

    u8g2->setCursor(42, 20);
    u8g2->println("-90db");

    // draw Ico
    // u8g2->drawGlyph(0, 9, 0x0e21a);
    // u8g2->drawGlyph(12, 9, 0x0e200);

    // this->draw_ico_connected(24);
    // this->draw_ico_usb(36);

    // LOCK/READY // TODO 锁定解锁控制
    u8g2->setFont(u8g2_font_12x6LED_tf);
    u8g2->setCursor(0, 32);
    u8g2->println("READY");
    // u8g2->println("LOCK");

    this->render_control_data();

    // u8g2->setFont(u8g2_font_9x6LED_tf);
    // auto w = 128 - u8g2->getStrWidth("12.75V");
    // u8g2->setCursor(w, 27);

    // u8g2->print("12.75V");
  };
};

BasePage *create_page_main()
{
  return new MainPage;
}
