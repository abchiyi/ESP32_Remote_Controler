#include "setting_devces.h"
#include "WouoUI.h"
#include "radio.h"

#define TAG "Page devices"

auto CONFIG_KEY_RADIO = "test";
auto main_k = "test__st";

LIST_VIEW Setting_devces_view;

class L_DEVICES : public ListPage
{
public:
    void render()
    {

        Setting_devces_view = {
            {"[ Device ]", create_page_jump_fn()},
            {"- Manage devices"},
        };

        for (const auto &mac_array : RADIO.get_copy())
        {
            char macStr[18]; // 用于存储 MAC 地址字符串的数组
            snprintf(macStr, sizeof(macStr), MACSTR, MAC2STR(mac_array));
            std::string macString = std::string("- ") + macStr;
            Setting_devces_view.push_back({macString,
                                           [&](WouoUI *ui)
                                           {
                                               RADIO.remove(mac_array);
                                               ESP_LOGI(TAG, "Remove");
                                           }});
        }

        ListPage::render();
    }

    L_DEVICES(LIST_VIEW &_view) : ListPage(_view) {};
};

BasePage *create_page_devices()
{
    return new L_DEVICES(Setting_devces_view);
}