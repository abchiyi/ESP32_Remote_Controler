#ifndef CONFIG_H
#define CONFIG_H
#include "Arduino.h"
#include "XBOX.h"

enum TASK_PRIORITY
{
    TP_L = 10,      // Lowest
    TP_N = 8,       // Normal
    TP_H = 2,       // High
    TP_HIGHEST = 0, // Highest
};
typedef enum
{
    ESP_NOW,
    BT_CONTROLLER,
    __radio_mode_max
} radio_mode_t;
typedef enum
{
    SLAVE,
    MASTER,
} control_mode_t;

#define WIFI_SSID_MAX 32
#define WIFI_PASS_MAX 64
#define CONFID_DATA_SIZE 256 // 配置数据大小
typedef class Config
{
private:
    uint8_t previous_raw[CONFID_DATA_SIZE];       // 存储上一次的raw值
    static void configCheckTask(void *parameter); // 定时检查任务
    TaskHandle_t checkTaskHandle;                 // 任务句柄
    uint8_t calculateChecksum(uint8_t *, size_t); // 计算校验和

public:
    union
    {
        struct
        {
            uint8_t data_is_alive;         // 数据是否有效
            char WIFI_SSID[WIFI_SSID_MAX]; // Wi-Fi SSID
            char WIFI_PASS[WIFI_PASS_MAX]; // Wi-Fi 密码
            radio_mode_t radio_mode;       // 无线模式
            control_mode_t control_mode;   // 控制模式

            XBOX_INPUT_t ROLL;       // 滚转
            XBOX_INPUT_t PITCH;      // 俯仰
            XBOX_INPUT_t YAW;        // 偏航
            XBOX_INPUT_t THRUST;     // 推力
            XBOX_INPUT_t breaker[2]; // 制动，使用数组以组合按钮模式
            XBOX_INPUT_t Reverse;    // 反转推力

            // 对应通道的翻转值开关配置
            uint8_t ROLL_FLIP;       // 滚转翻转
            uint8_t PITCH_FLIP;      // 俯仰翻转
            uint8_t YAW_FLIP;        // 偏航翻转
            uint8_t THRUST_FLIP;     // 推力方向翻转
            uint8_t breaker_FLIP[2]; // 制动翻转开关
            uint8_t Reverse_FLIP;    // 反转推力开关
        } __attribute__((packed));

        uint8_t raw[CONFID_DATA_SIZE];
    };

    void begin();

    Config() : radio_mode(ESP_NOW), control_mode(SLAVE) {};
    ~Config(); // 析构函数用于清理任务

    bool save();
    void print(); // 打印当前配置信息
} config_t;

extern config_t CONFIG; // 全局配置变量
#define CONFIG_VERSION 1

#endif
