#include <view/fidgetToy.h>
#include <view/setting.h>
#include <view/editor.h>
#include <view/sleep.h>
#include <view/menu.h>
#include <view/mainPage.h>

#include <esp_log.h>
LIST_VIEW Menu_view{
    {"[ Main ]"},
    {"- F0toy"},
    {"- Editor"},
    {"- Setting"},
};

class TP_MENU : public ListPage
{
protected:
public:
  void create()
  {
    this->setPageView("menu", Menu_view);
  }

  void router(uint8_t selectItmeNumber)
  {
    switch (selectItmeNumber)
    {
    case 0:
      // gui->page_out_to(P_SLEEP);
      gui->page_out_to(P_MAIN);
      break;
    case 1:
      gui->page_in_to(F0TOY);
      break;
    case 2:
      gui->page_in_to(P_EDITOR);
      break;
    case 3:
      gui->page_in_to(P_SETTING);
      break;
    }
  }
};

ListPage *P_MENU = new TP_MENU;
