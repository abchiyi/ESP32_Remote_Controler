#include "setting_devces.h"
#include "WouoUI.h"
#include "view/mainPage.h"
#include "radio.h"

#define TAG "Page devices"

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
            {"- Manage devices"},
        };

        // 新的MAC地址值
        mac_t new_device1 = {0xA1, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
        mac_t new_device2 = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB};
        mac_t new_device3 = {0xC1, 0xD2, 0xE3, 0xF4, 0xA5, 0xB6};

        // 更新向量
        CONFIG_RADIO.paired_devices.push_back(new_device1);
        CONFIG_RADIO.paired_devices.push_back(new_device2);
        CONFIG_RADIO.paired_devices.push_back(new_device3);

        for (const auto &array : CONFIG_RADIO.paired_devices)
        {
            ESP_LOGI(TAG, "" MACSTR "", MAC2STR(array));
        }

        ESP_LOGI(TAG, "%d", CONFIG_RADIO.paired_devices.size());
    };

    L_DEVICES(LIST_VIEW &_view) : ListPage(_view){};
};

BasePage *P_DEVICES = new L_DEVICES(Setting_devces_view);