#include <view/mainPage.h>
// #include <view/menu.h>
#include <controller.h>
#include <radio.h>
#include <car.h>
#include <view/setting_devces.h>

#define slider_width 6
#define slider_length 45

#define TAG "Main page"

class MainPage : public BasePage
{

public:
  void create()
  {
    this->name = "Main page";
    ESP_LOGI(this->name, "page index is %d", this->index);
  };

  void before()
  {
    ESP_LOGI(TAG, "init page");
    u8g2->setFont(LIST_FONT);
    u8g2->setDrawColor(2);
  }

  void onUserInput(int8_t btnID)
  {
    switch (btnID)
    {
    case BTN_ID_MENU:
      // gui->page_in_to(P_MENU);
      gui->page_in_to(P_DEVICES);
      break;
    }
  };

  void render_channel_view()
  {
    // TODO 通道数据可配置，而非固定
    float joy_l_x = Controller.joyLHori / 2048.0;
    float joy_l_y = Controller.joyLVert / 2048.0;

    float joy_r_x = Controller.joyRHori / 2048.0;
    float joy_r_y = Controller.joyRVert / 2048.0;

    // slider 1
    this->draw_slider_y(joy_l_y,
                        0, gui->DISPLAY_HEIGHT - slider_length,
                        slider_width, slider_length,
                        true);

    // slider 2
    this->draw_slider_x(joy_l_x,
                        10, gui->DISPLAY_HEIGHT - slider_width,
                        slider_width, slider_length,
                        true);

    // slider 3
    this->draw_slider_y(joy_r_y,
                        gui->DISPLAY_WIDTH - slider_width, gui->DISPLAY_HEIGHT - slider_length,
                        slider_width, slider_length,
                        true);

    // slider 4
    this->draw_slider_x(joy_r_x,
                        gui->DISPLAY_WIDTH - slider_length - 10, gui->DISPLAY_HEIGHT - slider_width,
                        slider_width, slider_length,
                        true);
  };

  void render()
  {
    this->render_channel_view();

    // u8g2->setCursor(0, 8);
    // u8g2->printf("%s | %s",
    //              radio.status == RADIO_CONNECTED ? "CONNECTED" : "DISCONNECT", radio.status == RADIO_PAIR_DEVICE ? "P--" : "---");

    // auto data = get_channel_status().channel[0];
    // auto gear = (data >> 2) & 0x03;
    // auto brake = data & 0x03;
    // auto value = (data >> 4);

    // auto gear_char = gear == 0   ? "R"
    //                  : gear == 1 ? "D"
    //                              : "N";
    // u8g2->printf("%S BRAKE-%s | %d", gear_char, brake ? "ON " : "OFF", value);
  };
};

BasePage *P_MAIN = new MainPage;