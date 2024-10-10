
#include "Arduino.h"
#include "power.h"
#include "pins.h"
#include "esp_log.h"

#define TAG "PMU"

XPowersPMU PMU;
power Power;

bool pmu_flag = 0;

void setFlag(void)
{
  pmu_flag = true;
}

void power::begin()
{
  if (!PMU.begin(Wire, 0x34, SDA, SCL))
  {
    ESP_LOGE(TAG, "PMU IS NOT ONLINE...");
    while (1)
      delay(50);
  }
  ESP_LOGI(TAG, "PMU ID 0x%x\n", PMU.getChipID());
  PMU.setSysPowerDownVoltage(2700);

  PMU.setVbusVoltageLimit(XPOWERS_AXP202_VBUS_VOL_LIM_4V5);
  PMU.setVbusCurrentLimit(XPOWERS_AXP202_VBUS_CUR_LIM_OFF);
  PMU.setDC2Voltage(2200);  // DC2 700~2275 mV, 25mV/step，IMAX=1.6A;
  PMU.setDC3Voltage(3300);  // DC3 700~3500 mV, 25mV/step，IMAX=1.2A;
  PMU.setLDO2Voltage(3300); // LDO2 1800~3300 mV, 100mV/step, IMAX=200mA
  PMU.setLDO3Voltage(1800); // LDO3 700~3500 mV, 25mV/step, IMAX=200mA

  /*  LDO4 Range:
      1250, 1300, 1400, 1500, 1600, 1700, 1800, 1900,
      2000, 2500, 2700, 2800, 3000, 3100, 3200, 3300
  */
  PMU.setLDO4Voltage(3100);

  // LDOio 1800~3300 mV, 100mV/step, IMAX=50mA
  PMU.setLDOioVoltage(3300);

  // Enable power output channel
  PMU.enableDC2();
  PMU.disableDC2();
  PMU.enableDC3();
  PMU.enableLDO2();
  PMU.enableLDO3();
  PMU.enableLDO4();
  PMU.enableLDOio();

  //
  PMU.enableExternalPin();

  ESP_LOGI(TAG, "EXTERN ON %d", PMU.isEnableExternalPin());
  ESP_LOGI(TAG, "BTE V  %d", PMU.getBattVoltage());

  ESP_LOGI(TAG,
           "DCDC=======================================================================");
  ESP_LOGI(TAG, "DC2  :%s   Voltage:%u mV \n", PMU.isEnableDC2() ? "ENABLE" : "DISABLE", PMU.getDC2Voltage());
  ESP_LOGI(TAG, "DC3  :%s   Voltage:%u mV \n", PMU.isEnableDC3() ? "ENABLE" : "DISABLE", PMU.getDC3Voltage());
  ESP_LOGI(TAG, "LDO=======================================================================");
  ESP_LOGI(TAG, "LDO2: %s   Voltage:%u mV\n", PMU.isEnableLDO2() ? "ENABLE" : "DISABLE", PMU.getLDO2Voltage());
  ESP_LOGI(TAG, "LDO3: %s   Voltage:%u mV\n", PMU.isEnableLDO3() ? "ENABLE" : "DISABLE", PMU.getLDO3Voltage());
  ESP_LOGI(TAG, "LDO4: %s   Voltage:%u mV\n", PMU.isEnableLDO4() ? "ENABLE" : "DISABLE", PMU.getLDO4Voltage());
  ESP_LOGI(TAG, "LDOio: %s   Voltage:%u mV\n", PMU.isEnableLDOio() ? "ENABLE" : "DISABLE", PMU.getLDOioVoltage());
  ESP_LOGI(TAG, "==========================================================================");
  // while (1)
  // {
  //   auto a = PMU.getBatteryPercent();

  //   ESP_LOGI(TAG, "battery P %d, %dV, %.2fmv",
  //            PMU.getBatteryPercent(), PMU.getBattVoltage(), PMU.getBattDischargeCurrent());
  //   delay(500);
  //   /* code */
  // }
};
