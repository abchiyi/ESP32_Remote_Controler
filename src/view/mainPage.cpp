#include <view/mainPage.h>
#include <view/menu.h>
#include <controller.h>
#include <radio.h>

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
    u8g2->setCursor(2, 10);
    // u8g2->printf("RightHatX :%d \n", Xbox.getAnalogHat(RightHatX));
    u8g2->print("Radio  status :");
    u8g2->setCursor(2, 20);
    u8g2->print(radio.status == RADIO_CONNECTED ? "CONNECTED" : "DISCONNECT");
    u8g2->setCursor(2, 34);
    u8g2->print("Radio pair device :");
    u8g2->setCursor(2, 44);
    u8g2->print(radio.status == RADIO_PAIR_DEVICE ? "RADIO_PAIR_DEVICE" : " -- ");
  };
};

BasePage *P_MAIN = new MainPage;