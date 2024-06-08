#include <view/mainPage.h>
#include <view/menu.h>
#include "view/setting_devces.h"
#include <view/setting.h>
// #include <view/fidgetToy.h>
// #include <view/editor.h>
// #include <view/sleep.h>

LIST_VIEW Menu_view{
    {"[ Main ]", create_page_jump_fn(PAGE_OUT, P_MAIN)},
    {"- Set devices", create_page_jump_fn(PAGE_IN, P_DEVICES)},
    {"- F0toy"},
    {"- Editor"},
    {"- Setting", create_page_jump_fn(PAGE_IN, P_SETTING)},
};

class P_LMENU : public ListPage
{
public:
  void before()
  {
    ListPage::before();
  };
  P_LMENU(LIST_VIEW &_view) : ListPage(_view){};
};

BasePage *P_MENU = new P_LMENU(Menu_view);
