#include <view/sleep.h>
#include <view/menu.h>

class Sleep : public BasePage
{
private:
public:
  void before()
  {
    gui->add_event_listener(KEY_WAKE, [&]()
                            { gui->gui_awake(); });
    gui->gui_sleep();
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