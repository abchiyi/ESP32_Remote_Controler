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
    void before()
    {
        if (RADIO.status != RADIO_CONNECTED)
        {
            ESP_LOGI(TAG, "before&scan AP");
            RADIO.status = RADIO_IN_SCAN_BEFORE;
        }
    };

    void render()
    {

        Setting_devces_view = {
            {"[ Device ]", create_page_jump_fn()},
            {"- Manage devices"},
        };

        if (RADIO.status != RADIO_IN_SCAN)
            for (const auto &ap : RADIO.AP)
            {
                char macStr[18]; // 用于存储 MAC 地址字符串的数组
                snprintf(macStr, sizeof(macStr), MACSTR, MAC2STR(ap.MAC));
                std::string macString = std::string("- ") + macStr;
                Setting_devces_view.push_back(
                    {String("- " + ap.SSID).c_str(),
                     [=](WouoUI *ui)
                     {
                         ESP_LOGI(TAG, "connect to %s", ap.SSID.c_str());
                         RADIO.connect_to(&ap);
                     },
                     create_render_content(&ap.RSSI)});
            }

        ListPage::render();
    }

    void leave()
    {
        RADIO.AP.clear();  
    }

    L_DEVICES(LIST_VIEW &_view) : ListPage(_view) {};
};

BasePage *create_page_devices()
{
    return new L_DEVICES(Setting_devces_view);
}