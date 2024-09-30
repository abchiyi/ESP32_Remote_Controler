#include "setting_devces.h"
#include "WouoUI.h"
#include "radio.h"
#include "algorithm"

#define TAG "Page devices"

std::vector<ap_info_t> AP_INFO;

radio_cb_fn_t up_date_cb_fn = [&]()
{
    ESP_LOGI(TAG, "up date sizeof %d", RADIO.AP.size());
    AP_INFO.assign(RADIO.AP.begin(), RADIO.AP.end());
};

TimerHandle_t xTimer;
void vTimerCallback(TimerHandle_t xTimer)
{
    RADIO.status = RADIO_IN_SCAN_BEFORE;
};

LIST_VIEW Setting_devces_view;

class L_DEVICES : public ListPage
{
public:
    void before()
    {
        if (xTimer == NULL)
            esp_system_abort("Timer creation failed"); // 定时器创建失败
        else
            xTimerStart(xTimer, 1); // 启动定时器

        RADIO.status = RADIO_IN_SCAN_BEFORE;
    };

    void render()
    {

        Setting_devces_view = {
            {"[ Device ]", create_page_jump_fn()},
            {"- Manage devices"},
        };

        for (const auto &ap : AP_INFO)
        {
            char macStr[18]; // 用于存储 MAC 地址字符串的数组
            snprintf(macStr, sizeof(macStr), MACSTR, MAC2STR(ap.MAC));
            std::string macString = std::string("- ") + macStr;
            Setting_devces_view.push_back(
                {String("- " + ap.SSID).c_str(),
                 [=](WouoUI *ui)
                 {
                     ESP_LOGI(TAG, "connect to %s", ap.SSID.c_str());
                     xTimerStop(xTimer, 5);
                     RADIO.connect_to(&ap);
                 },
                 create_render_content(ap.RSSI)});
        }

        ListPage::render();
    }

    void leave()
    {
        xTimerStop(xTimer, 5);
        RADIO.adter_scan_ap_comp = nullptr;
        RADIO.AP.clear();
        AP_INFO.clear();
    }

    L_DEVICES(LIST_VIEW &_view) : ListPage(_view)
    {
        // 创建定时器
        xTimer = xTimerCreate(
            "Timer",             // 定时器名称
            pdMS_TO_TICKS(7000), // 定时器周期（以系统节拍为单位）
            pdTRUE,              // 自动重载
            (void *)0,           // 定时器 ID
            vTimerCallback       // 回调函数
        );

        AP_INFO.reserve(10);

        RADIO.adter_scan_ap_comp = up_date_cb_fn;
    };
};

BasePage *create_page_devices()
{
    return new L_DEVICES(Setting_devces_view);
}