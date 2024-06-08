#include "setting_devces.h"
#include "WouoUI.h"
#include "view/mainPage.h"

LIST_VIEW Setting_devces_view;

class L_DEVICES : public ListPage
{
public:
    void create()
    {
    }

    void before()
    {
        ListPage::before();

        Setting_devces_view = {
            {"[ Device ]", create_page_jump_fn(PAGE_OUT, P_MAIN)},
            {"- Add new Device"},
        };
    };

    L_DEVICES(LIST_VIEW &_view) : ListPage(_view){};
};

BasePage *P_DEVICES = new L_DEVICES(Setting_devces_view);