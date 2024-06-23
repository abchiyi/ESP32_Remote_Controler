#include <view/menu.h>
#include "view/setting_devces.h"
#include <view/setting.h>
#include <view/fidgetToy.h>
// #include <view/editor.h>
// #include <view/sleep.h>

LIST_VIEW Menu_view{
    {"[ Main ]", create_page_jump_fn()},
    {"- Set devices", create_page_jump_fn(create_page_devices)},
    {"- F0toy", create_page_jump_fn(create_page_f0_toy)},
    {"- Editor"},
    {"- Setting", create_page_jump_fn(create_page_setting)},
};

class P_LMENU : public ListPage
{
public:
  void before()
  {
    ListPage::name = "Menu";
    ListPage::before();
  };
  P_LMENU(LIST_VIEW &_view) : ListPage(_view) {};
};

BasePage *create_page_menu()
{
  return new P_LMENU(Menu_view);
}

BasePage *P_MENU = new P_LMENU(Menu_view);
