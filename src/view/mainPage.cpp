#include <view/mainPage.h>
#include <view/menu.h>
#include <controller.h>

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
    u8g2->setCursor(10, 32);
    u8g2->printf("RightHatX :%d \n", Xbox.getAnalogHat(RightHatX));
  };
};

BasePage *P_MAIN = new MainPage;