#include <view/sleep.h>
#include <view/menu.h>

class Sleep : public BasePage
{
public:
  void before()
  {

    this->u8g2->setPowerSave(1); // 关闭屏幕
    // if (eeprom.change)
    // {
    //   eeprom.change = false;
    // }
  }

  void onUserInput(int8_t btnID)
  {
    switch (btnID)
    {
    case BTN_ID_DO:
      break; // 顺时针旋转执行的函数
    case BTN_ID_UP:
      break; // 逆时针旋转执行的函数
    case BTN_ID_CONFIRM:
      break;
    case BTN_ID_CANCEL:
      gui->page_in_to(create_page_menu);
      // list.box_y = 0;
      // list.box_w = 0;
      // list.box_w_trg = 0;
      // list.box_h = 0;
      // list.box_h_trg = 0;
      // list.bar_h = 0;
      u8g2->setPowerSave(0);
      break;
    }
  }

  void render()
  {
  } // 睡眠页不需要显示内容
};

BasePage *P_SLEEP = new Sleep;