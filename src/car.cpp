#include "controller.h"
#include "radio.h"
#include "car.h"

enum break_key
{
  BRAKE_LT,
  BRAKE_RT
};

SemaphoreHandle_t xMutex;

radio_data_t data_to_send;

/**
 * @brief 输出一个高低位合并的数值
 * @param value1 12bit特数值
 * @param value2 4 bit数值
 */
uint16_t set_combined_int(uint16_t value1, uint16_t value2)
{
  return (value1 << 4) | value2;
}

// 挡位状态
typedef enum gear
{
  REVERSE,
  FORWARD,
  SLIDE,
  BRAKE
} gear_t;

/* 前照灯 */
int HeadLight = 0;          // 进/远光/示宽/前雾灯复用灯
bool DISTANT_LIGHT = false; // 远光
bool LOW_BEAM = false;      // 近光

/* 尾灯 */
int StopLight = 0;       // 刹车/示宽/后雾灯复用灯
bool WIDTH_LAMP = false; // 示宽灯

int REVERSING_LIGHT = 0; // 倒车灯

/* 左右闪光灯组 */
bool LightTurnL = false;  // 左转灯
bool LightTurnR = false;  // 右转灯
bool HazardLight = false; // 危险报警灯

void set_channel(radio_data_t *data)
{
  static int BRAKE_KEY;
  static bool HOLD_RT;
  static bool HOLD_LT;
  static bool changed;
  static bool brake;

  auto LT = Controller.trigLT;
  auto RT = Controller.trigRT;

  // 确定前进方向;
  if (LT || RT)
  {
    if (!changed)
    {
      HOLD_RT = (bool)RT;
      HOLD_LT = !HOLD_RT ? (bool)LT : false;
      BRAKE_KEY = HOLD_RT ? 0 : 1;
      changed = true;
    }
  }
  else
  {
    HOLD_RT = false;
    HOLD_LT = false;
    changed = false;
    changed = !LT && !RT ? false : changed;
  }

  brake = BRAKE_KEY == BRAKE_RT ? RT : LT;

  REVERSING_LIGHT = HOLD_LT ? 150 : 0; // 倒车灯

  // ESP_LOGI(TAG, "Hold LT :%d, Hold RT :%d, Brake :%d", HOLD_LT, HOLD_RT, brake);

  auto gear = HOLD_RT   ? FORWARD
              : HOLD_LT ? REVERSE
                        : SLIDE;

  auto value = gear == FORWARD ? RT : LT;

  if (xSemaphoreTake(xMutex, 4) == pdTRUE)
  {
    data->channel[0] = (value << 4) | (gear << 2) | brake;
    data->channel[1] = set_combined_int(Controller.joyLHori, gear);
    xSemaphoreGive(xMutex);
  }
}

void task_controll_main(void *pt)
{
  TickType_t xLastWakeTime = xTaskGetTickCount(); // 最后唤醒时间
  const TickType_t xFrequency = pdMS_TO_TICKS(8); // 设置采样率 250hz

  while (true)
  {
    if (radio.status == RADIO_CONNECTED && Xbox.XboxOneConnected)
    {
      set_channel(&data_to_send);
      radio.set_data(&data_to_send);
    }
    xTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

void car_controll_start()
{
  xMutex = xSemaphoreCreateMutex();

  xTaskCreate(task_controll_main, "main_controller", 4096, NULL, 1, NULL);
}

radio_data_t get_channel_status()
{
  if (xSemaphoreTake(xMutex, 4) == pdTRUE)
  {
    radio_data_t data = data_to_send;
    xSemaphoreGive(xMutex);
    return data;
  }
}