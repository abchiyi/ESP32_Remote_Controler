#include <view/about.h>
#include <view/setting.h>

M_SELECT about_menu[]{
    {"[ WoW-UI ]"},
    {"- Version: v1.0"},
    {"- Creator: RQNG"},
    {"- Bili UID: 9182439"},
};

class About : public ListPage
{

public:
  void create()
  {
    this->setPageView("About", about_menu);
  }

  void router(uint8_t selectItmeNumber)
  {
    switch (selectItmeNumber)
    {
    case 0:
      this->gui->page_out_to(P_SETTING);
      break;
    }
  };
};

ListPage *P_ABOUT = new About();