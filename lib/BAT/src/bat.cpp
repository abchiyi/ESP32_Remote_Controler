#include "bat.h"
#include "config.h"
#include "tool.h"

#include "Arduino.h"
#include "atomic"

#define TAG "BAT"

#define BAT_PIN 2          // ADC IO
#define VOT_X 4            // 电压衰减倍率
#define ALPHA 0.3f         // 低通滤波系数(0.1~0.3)
#define VOLTAGE_FULL 4200  // 单节锂电池满电电压（mV）
#define VOLTAGE_EMPTY 3000 // 单节锂电池空电电压（mV）

static const TickType_t xFrequency = HZ2TICKS(50); // 采样率/HZ
static std::atomic<uint32_t> BAT_MV;               // 电池电压
static uint8_t BAT_CELL = 0;                       // 电池串联数量
static float bat_low_vot = 0;                      // 电池低电压阈值

/**
 * @brief 检测锂电池串联数量（1-3串）
 * @param total_voltage_mv 总电压（单位：mV）
 * @return 电池串数（1-3），0表示无效
 */
uint8_t detect_cell_count(uint32_t total_voltage_mv)
{
    // 单节锂电池电压范围（单位：mV）
    const uint32_t CELL_MIN_MV = 2500; // 2.5V = 2500mV
    const uint32_t CELL_MAX_MV = 4350; // 4.35V = 4350mV

    // 检查1-3串的可能性（全部用整数运算）
    for (uint8_t cells = 1; cells <= 3; cells++)
    {
        // 计算当前串数的合理范围（±10%容差）
        uint32_t min_valid = (cells * CELL_MIN_MV * 9) / 10;  // cells*CELL_MIN*0.9
        uint32_t max_valid = (cells * CELL_MAX_MV * 11) / 10; // cells*CELL_MAX*1.1

        if (total_voltage_mv >= min_valid && total_voltage_mv <= max_valid)
            return cells;
    }
    return 0; // 无效电压
}

void task_read_vot(void *pt)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    uint32_t filtered_voltage = 0; // 低通滤波中间变量
    pinMode(BAT_PIN, INPUT_PULLDOWN);
    while (true)
    {
        uint32_t raw_voltage = analogReadMilliVolts(BAT_PIN);

        filtered_voltage = ALPHA * raw_voltage + (1 - ALPHA) * filtered_voltage;
        BAT_MV.store(filtered_voltage * VOT_X);

        ESP_LOGD(TAG, "Bat_V: %.2fV (Raw: %.2fV), %d Cell",
                 BAT_MV / 1000.0f,
                 raw_voltage / 1000.0f,
                 detect_cell_count(BAT_MV));

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void init_bat()
{
    xTaskCreate(task_read_vot, "task_read_vot", 1024 * 3, NULL, TP_L, NULL);

    vTaskDelay(300); // 等待检测电压稳定

    // 检测电池串联数量,
    // while (!BAT_CELL)
    // {
    BAT_CELL = detect_cell_count(BAT_MV.load());
    //     vTaskDelay(10);
    // }
    ESP_LOGI(TAG, "BAT_CELL:%d", BAT_CELL);
}

uint32_t get_bat_mv()
{
    return BAT_MV.load();
}

uint8_t get_bat_cell()
{
    return BAT_CELL;
}
