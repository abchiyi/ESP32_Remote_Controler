#include <view/about.h>
#include "view/setting_devces.h"
// #include <view/setting.h>

LIST_VIEW about_menu{
    {"[ WoW-UI ]", [&](WouoUI *gui)
     {
       gui->page_out_to(P_DEVICES);
     }},
    {"- Version: v1.0"},
    {"- Creator: RQNG"},
    {"- Bili UID: 9182439"},
};

class About : public ListPage
{

public:
  void create()
  {
  }

  About(LIST_VIEW &_view) : ListPage(_view){};
};

ListPage *P_ABOUT = new About(about_menu);