#include "storage_config.h"

storage_config STORAGE_CONFIG;

#define TAG "STORAGE_CONFIG"

#define EEPROM_SIZE 512 // ESP32 EEPROM 容量

// 自动保存任务
void TASK_AUTO_SAVE(void *pt)
{
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFreequency = 500;
  while (true)
  {
    STORAGE_CONFIG.write_all();
    EEPROM.commit();
    vTaskDelayUntil(&xLastWakeTime, xFreequency);
  }
}

void storage_config::begin()
{
  EEPROM.begin(EEPROM_SIZE);
  this->read_all();
  xTaskCreate(TASK_AUTO_SAVE, "TASK_AUTO_SAVE", 1024 * 4, NULL, 1, NULL);
}
