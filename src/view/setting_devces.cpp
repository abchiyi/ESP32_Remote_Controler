#include "setting_devces.h"
#include "WouoUI.h"
#include "radio.h"
#include "algorithm"
#include "WiFi.h"
#include "Udp.h"
#include "esp_mac.h"
#include "controller.h"

WiFiUDP udp;

#define UPLOAD_PORT 2390

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

#define CRTP_MAX_DATA_SIZE 30

typedef struct
{

    union
    {
        uint8_t header; //< Header selecting channel and port
        struct
        {
#ifndef CRTP_HEADER_COMPAT
            uint8_t channel : 2; //< Selected channel within port
            uint8_t reserved : 2;
            uint8_t port : 4; //< Selected port
#else
            uint8_t channel : 2;
            uint8_t port : 4;
            uint8_t reserved : 2;
#endif
        };
    };

    float ROLL;      // 4 字节
    float PITCH;     // 4 字节
    float YAW;       // 4 字节
    uint16_t THRUST; // 2 字节}

    uint8_t __reserved[16];
} __attribute__((packed)) cpr;

typedef struct _CRTPPacket
{
    uint8_t size; //< Size of data
    union
    {
        struct
        {
            union
            {
                uint8_t header; //< Header selecting channel and port
                struct
                {
#ifndef CRTP_HEADER_COMPAT
                    uint8_t channel : 2; //< Selected channel within port
                    uint8_t reserved : 2;
                    uint8_t port : 4; //< Selected port
#else
                    uint8_t channel : 2;
                    uint8_t port : 4;
                    uint8_t reserved : 2;
#endif
                };
            };

            uint8_t data[CRTP_MAX_DATA_SIZE]; //< Data
        };

        uint8_t raw[CRTP_MAX_DATA_SIZE + 1]; //< The full packet "raw"
    };
} __attribute__((packed)) CRTPPacket;

static uint8_t calculate_cksum(void *data, size_t len)
{
    int i;
    unsigned char cksum = 0;
    auto c = (unsigned char *)data;

    for (i = 0; i < len; i++)
        cksum += *(c++);

    return cksum;
}

float match_angl(uint8_t angl, int16_t joy)
{
    auto step = angl / 2048.f;
    return joy * step;
};

void task_udp(void *)
{
    char incomingPacket[255]; // 缓存收到的数据
    while (true)
    {
        auto _raw_d = cpr();
        _raw_d.THRUST = Controller.joyLVert * 32;
        _raw_d.YAW = match_angl(40, Controller.joyLHori);
        _raw_d.PITCH = match_angl(30, Controller.joyRVert) * -1;
        _raw_d.ROLL = match_angl(30, Controller.joyRHori);
        _raw_d.channel = 0;
        _raw_d.port = 3;

        // auto pack = CRTPPacket();
        // memcpy(pack.raw, &_raw_d.raw, sizeof(_raw_d));

        uint8_t data[32];
        memcpy(data, &_raw_d, sizeof(_raw_d));
        data[31] = calculate_cksum(&_raw_d, 31);

        ESP_LOGI("TX DATA", "T %d", _raw_d.THRUST);

        udp.beginPacket("192.168.43.42", 2390);
        udp.write(data, sizeof(data));
        udp.endPacket();
        vTaskDelay(8);
    }
};

LIST_VIEW Setting_devices_view;
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

        Setting_devices_view = {
            {"[ Device ]", create_page_jump_fn()},
            {"- Manage devices"},
        };

        for (const auto &ap : AP_INFO)
        {
            char macStr[18]; // 用于存储 MAC 地址字符串的数组
            snprintf(macStr, sizeof(macStr), MACSTR, MAC2STR(ap.MAC));
            std::string macString = std::string("- ") + macStr;
            Setting_devices_view.push_back(
                {String("- " + ap.SSID).c_str(),
                 [=](WouoUI *ui)
                 {
                     //  ESP_LOGI(TAG, "connect to %s", ap.SSID.c_str());
                     //  xTimerStop(xTimer, 5);
                     //  RADIO.connect_to(&ap);
                     WiFi.begin(ap.SSID, "12345678");
                     udp.begin(UPLOAD_PORT);
                     xTaskCreate(task_udp, "task_udp", 1024 * 5, NULL, 5, NULL);
                 },
                 create_render_content(ap.RSSI)});
        }

        ListPage::render();
    }

    void leave()
    {
        xTimerStop(xTimer, 5);
        RADIO.cb_fn_on_scan_comp = nullptr;
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

        RADIO.cb_fn_on_scan_comp = up_date_cb_fn;
    };
};

BasePage *create_page_devices()
{
    return new L_DEVICES(Setting_devices_view);
}
