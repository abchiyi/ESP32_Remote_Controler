#include <view/mainPage.h>
#include <view/menu.h>
#include "view/setting_devces.h"
#include <view/setting.h>
#include <view/fidgetToy.h>
// #include <view/editor.h>
// #include <view/sleep.h>

LIST_VIEW Menu_view{
    {"[ Main ]", create_page_jump_fn(PAGE_OUT, create_page_main)},
    {"- Set devices", create_page_jump_fn(PAGE_IN, create_page_devices)},
    {"- F0toy", create_page_jump_fn(PAGE_IN, create_page_f0_toy)},
    {"- Editor"},
    {"- Setting", create_page_jump_fn(PAGE_IN, create_page_setting)},
};

class P_LMENU : public ListPage
{
public:
  P_LMENU(LIST_VIEW &_view) : ListPage(_view) {};
};

BasePage *create_page_menu()
{
  return new P_LMENU(Menu_view);
}

BasePage *P_MENU = new P_LMENU(Menu_view);
