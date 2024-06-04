#include "storage_config.h"

storage_config STORAGE_CONFIG;

#define TAG "STORAGE_CONFIG"

#define EEPROM_SIZE 512 // ESP32 EEPROM 容量

SemaphoreHandle_t __xMutex = xSemaphoreCreateMutex();
int16_t ADDR_START = 0;

// 自动保存任务
void TASK_AUTO_SAVE(void *pt)
{
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFreequency = 500;
  while (true)
  {
    STORAGE_CONFIG.write_all();
    vTaskDelayUntil(&xLastWakeTime, xFreequency);
  }
}

void storage_config::begin()
{
  EEPROM.begin(EEPROM_SIZE);
  // this->read_all();
  // xTaskCreate(TASK_AUTO_SAVE, "TASK_AUTO_SAVE", 1024 * 4, NULL, 1, NULL);
}
void storage_config::clearEEPROM()
{
  for (int i = 0; i < EEPROM_SIZE; i++)
    EEPROM.write(i, 0);
  EEPROM.commit();
}
