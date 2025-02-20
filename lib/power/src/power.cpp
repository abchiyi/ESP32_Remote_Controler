
#include "Arduino.h"
#include "power.h"
#include "pins.h"
#include "esp_log.h"

#define TAG "PMU"

XPowersPMU PMU;
power Power;

bool pmu_flag = 0;

void task_power(void *pt)
{

  while (true)
  {

    if (pmu_flag)
    {

      pmu_flag = false;

      // Get PMU Interrupt Status Register
      uint32_t status = PMU.getIrqStatus();
      Serial.print("STATUS => HEX:");
      Serial.print(status, HEX);
      Serial.print(" BIN:");
      Serial.println(status, BIN);

      if (PMU.isAcinOverVoltageIrq())
      {
        Serial.println("isAcinOverVoltageIrq");
      }
      if (PMU.isAcinInserIrq())
      {
        Serial.println("isAcinInserIrq");
      }
      if (PMU.isAcinRemoveIrq())
      {
        Serial.println("isAcinRemoveIrq");
      }
      if (PMU.isVbusOverVoltageIrq())
      {
        Serial.println("isVbusOverVoltageIrq");
      }
      if (PMU.isVbusInsertIrq())
      {
        Serial.println("isVbusInsertIrq");
      }
      if (PMU.isVbusRemoveIrq())
      {
        Serial.println("isVbusRemoveIrq");
      }
      if (PMU.isVbusLowVholdIrq())
      {
        Serial.println("isVbusLowVholdIrq");
      }
      if (PMU.isBatInsertIrq())
      {
        Serial.println("isBatInsertIrq");
      }
      if (PMU.isBatRemoveIrq())
      {
        Serial.println("isBatRemoveIrq");
      }
      if (PMU.isBattEnterActivateIrq())
      {
        Serial.println("isBattEnterActivateIrq");
      }
      if (PMU.isBattExitActivateIrq())
      {
        Serial.println("isBattExitActivateIrq");
      }
      // if (PMU.isBatChagerStartIrq())
      // {
      //   Serial.println("isBatChagerStartIrq");
      // }
      // if (PMU.isBatChagerDoneIrq())
      // {
      //   Serial.println("isBatChagerDoneIrq");
      // }
      if (PMU.isBattTempHighIrq())
      {
        Serial.println("isBattTempHighIrq");
      }
      if (PMU.isBattTempLowIrq())
      {
        Serial.println("isBattTempLowIrq");
      }
      if (PMU.isChipOverTemperatureIrq())
      {
        Serial.println("isChipOverTemperatureIrq");
      }
      if (PMU.isChargingCurrentLessIrq())
      {
        Serial.println("isChargingCurrentLessIrq");
      }
      if (PMU.isDC1VoltageLessIrq())
      {
        Serial.println("isDC1VoltageLessIrq");
      }
      if (PMU.isDC2VoltageLessIrq())
      {
        Serial.println("isDC2VoltageLessIrq");
      }
      if (PMU.isDC3VoltageLessIrq())
      {
        Serial.println("isDC3VoltageLessIrq");
      }
      if (PMU.isPekeyShortPressIrq())
      {
        Serial.println("isPekeyShortPress");

        // enterPmuSleep();

        // CHG LED mode test
        uint8_t m = PMU.getChargingLedMode();
        Serial.print("getChargingLedMode:");
        Serial.println(m++);
        m %= XPOWERS_CHG_LED_CTRL_CHG;
        Serial.printf("setChargingLedMode:%u", m);
        PMU.setChargingLedMode(m);
      }
      if (PMU.isPekeyLongPressIrq())
      {
        Serial.println("isPekeyLongPress");
      }
      if (PMU.isNOEPowerOnIrq())
      {
        Serial.println("isNOEPowerOnIrq");
      }
      if (PMU.isNOEPowerDownIrq())
      {
        Serial.println("isNOEPowerDownIrq");
      }
      if (PMU.isVbusEffectiveIrq())
      {
        Serial.println("isVbusEffectiveIrq");
      }
      if (PMU.isVbusInvalidIrq())
      {
        Serial.println("isVbusInvalidIrq");
      }
      if (PMU.isVbusSessionIrq())
      {
        Serial.println("isVbusSessionIrq");
      }
      if (PMU.isVbusSessionEndIrq())
      {
        Serial.println("isVbusSessionEndIrq");
      }
      if (PMU.isLowVoltageLevel2Irq())
      {
        Serial.println("isLowVoltageLevel2Irq");
      }
      if (PMU.isWdtExpireIrq())
      {
        Serial.println("isWdtExpire");

        // printPMU();
        // Clear the timer state and continue to the next timer
        PMU.clearTimerFlag();
      }
      if (PMU.isGpio2EdgeTriggerIrq())
      {
        Serial.println("isGpio2EdgeTriggerIrq");
      }
      if (PMU.isGpio1EdgeTriggerIrq())
      {
        Serial.println("isGpio1EdgeTriggerIrq");
      }
      if (PMU.isGpio0EdgeTriggerIrq())
      {
        Serial.println("isGpio0EdgeTriggerIrq");
      }
      // Clear PMU Interrupt Status Register
      PMU.clearIrqStatus();
    }
    vTaskDelay(8);
  }
};

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
  PMU.setPowerKeyPressOffTime(XPOWERS_AXP202_POWEROFF_4S);

  xTaskCreate(task_power, "POWER", 1024 * 2, NULL, 1, NULL);
  pinMode(10, INPUT);
  attachInterrupt(10, setFlag, FALLING);

  // while (1)
  // {
  //   auto a = PMU.getBatteryPercent();

  //   ESP_LOGI(TAG, "battery P %d, %dV, %.2fmv",
  //            PMU.getBatteryPercent(), PMU.getBattVoltage(), PMU.getBattDischargeCurrent());
  //   delay(500);
  //   /* code */
  // }
};
