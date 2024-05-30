#include "setting_devces.h"
#include "setting.h"
#include "WouoUI.h"

M_SELECT Setting_devces_view[]{
    {"[ Device ]"},
    {"- Add new Device"},    // TODO 二级菜单
    {"- All paired device"}, // TODO 二级菜单
};

class L_DEVICES : public ListPage
{
public:
    void create()
    {
        this->setPageView("Devices", Setting_devces_view);
    }

    void router(uint8_t selectItmeNumber)
    {
        switch (selectItmeNumber)
        {
        case 0:
            gui->page_out_to(P_SETTING);
            break;
        }
    }
};

ListPage *P_DEVICES = new L_DEVICES;