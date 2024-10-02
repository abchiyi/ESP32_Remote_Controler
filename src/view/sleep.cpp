#include <view/sleep.h>
#include <view/menu.h>

class Sleep : public BasePage
{
public:
  void before()
  {
    this->u8g2->setPowerSave(1); // 关闭屏幕
  }

  void onUserInput(int8_t btnID)
  {
    switch (btnID)
    {
    case KEY_DOWN:
      break; // 顺时针旋转执行的函数
    case KEY_UP:
      break; // 逆时针旋转执行的函数
    case KEY_CONFIRM:
      break;
    case KEY_BACK:
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

BasePage *page_sleep()
{
  return new Sleep;
}