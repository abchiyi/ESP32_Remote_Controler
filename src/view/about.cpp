#include <view/about.h>
#include <view/setting.h>

LIST_VIEW about_menu{
    {"[ WoW-UI ]", create_page_jump_fn(PAGE_OUT, create_page_setting)},
    {"- Version: v1.0"},
    {"- Creator: RQNG"},
    {"- Bili UID: 9182439"},
};

class About : public ListPage
{

public:
  About(LIST_VIEW &_view) : ListPage(_view) {};
};

BasePage *create_page_about()
{
  return new About(about_menu);
}

BasePage *P_ABOUT = new About(about_menu);