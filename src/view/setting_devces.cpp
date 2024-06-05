#include "setting_devces.h"
#include "WouoUI.h"
#include "view/mainPage.h"

// auto a = [&](WouoUI *gui)
// {
//     ESP_LOGI("gui test", "test view cb_fn");
// };

LIST_VIEW Setting_devces_view{
    {"[ Device ]",
     [&](WouoUI *ui)
     {
         ui->page_out_to(P_MAIN);
     }},

    {"- Add new Device"},    // TODO 二级菜单
    {"- All paired device"}, // TODO 二级菜单
};

class L_DEVICES : public ListPage
{
public:
    void create()
    {
    }

    void before()
    {
        ListPage::before();
        Setting_devces_view.push_back({"- test uint 3"});
    };

    L_DEVICES(LIST_VIEW &_view) : ListPage(_view){};
};

ListPage *P_DEVICES = new L_DEVICES(Setting_devces_view);