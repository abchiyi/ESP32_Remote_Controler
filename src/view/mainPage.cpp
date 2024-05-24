#include <view/mainPage.h>
#include <view/menu.h>
#include <controller.h>
#include <radio.h>
#include <car.h>

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
    u8g2->setFont(LIST_FONT);
    u8g2->setDrawColor(2);
    ESP_LOGI(this->name, "set u8g2 font");
  }

  void onUserInput(int8_t btnID)
  {
    switch (btnID)
    {
    case BTN_ID_MENU:
      gui->page_in_to(P_MENU);
      break;
    }
  };

  void render()
  {
    u8g2->setCursor(0, 8);
    u8g2->printf("%s | %s",
                 radio.status == RADIO_CONNECTED ? "CONNECTED" : "DISCONNECT", radio.status == RADIO_PAIR_DEVICE ? "P--" : "---");

    float v = Controller.joyLHori / 2048.0;  // 百分比
    float v2 = Controller.joyLVert / 2048.0; // 百分比
    u8g2->setCursor(0, 20);
    u8g2->printf("X %.2f ", v);
    u8g2->setCursor(0, 30);
    u8g2->printf("Y %.2f", v2);

    uint8_t width = 60; // 进度条宽度
    uint8_t height = 4; // 进度条高度
    uint8_t x = 0;      // 进度条x坐标
    uint8_t y = 4;

    this->draw_slider_x(v, 2, 64 - (height * 4 + 4), height, 61);
    this->draw_slider_x(v, 2, 64 - (height * 3 + 3), height, 61, true);

    this->draw_slider_x(v, 2, 64 - (height * 2 + 2), height, width);
    this->draw_slider_x(v, 2, 64 - height, height, width, true);

    this->draw_slider_y(v2, 70, 15, 40, 4, true);
    this->draw_slider_y(v2, 80, 15, 41, 4, true);
    this->draw_slider_y(v2, 90, 15, 40, 4);
    this->draw_slider_y(v2, 100, 15, 41, 4);

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