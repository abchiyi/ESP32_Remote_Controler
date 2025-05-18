#ifndef CONFIG_H
#define CONFIG_H
#include "Arduino.h"

enum TASK_PRIORITY
{
    TP_L = 10,      // Lowest
    TP_N = 8,       // Normal
    TP_H = 2,       // High
    TP_HIGHEST = 0, // Highest
};
typedef enum
{
    ESP_NOW_AIR,
    ESP_NOW_GROUND,
} radio_mode_t;

#define WIFI_SSID_MAX 32
#define WIFI_PASS_MAX 64

typedef class Config
{
private:
    uint8_t previous_raw[128];                    // 存储上一次的raw值
    static void configCheckTask(void *parameter); // 定时检查任务
    TaskHandle_t checkTaskHandle;                 // 任务句柄
    uint8_t calculateChecksum(uint8_t *, size_t); // 计算校验和

public:
    union
    {
        struct
        {
            uint8_t data_is_alive;                 // 数据是否有效
            char WIFI_SSID[WIFI_SSID_MAX];         // Wi-Fi SSID
            char WIFI_PASS[WIFI_PASS_MAX];         // Wi-Fi 密码
            radio_mode_t radio_mode = ESP_NOW_AIR; // 无线模式
        };
        uint8_t raw[128];
    };

    void begin();
    ~Config(); // 析构函数用于清理任务

    bool save();
} config_t;

extern config_t CONFIG; // 全局配置变量
#define CONFIG_VERSION 1

#endif
