#include <display.h>
#include <esp_log.h>

#define TAG "Display"

struct dataAndDisplay
{
  Adafruit_SSD1306 *display;
  Radio *radio;
};

void TaskDisplayMain(void *pt)
{
  dataAndDisplay data = *(dataAndDisplay *)pt;
  while (true)
  {
    /* code */
    vTaskDelay(16);
  }
}
void TaskDisplaySub(void *pt)
{
  ESP_LOGI(TAG, "task sub display get data");
  auto [display, radio] = *(dataAndDisplay *)pt;
  ESP_LOGI(TAG, "task sub display start update");
  while (true)
  {
    display->clearDisplay();
    display->setCursor(0, 0);
    display->write(*radio->isPaired ? 0x12 : 0x18);
    display->display();
    vTaskDelay(16);
  }
}

void Display::begin(Radio *radio)
{
  ESP_LOGI(TAG, "Init display main");
  displayMain = Adafruit_SSD1306(128, 64, DI, DO, DC, RES, CS);
  if (!displayMain.begin(SSD1306_SWITCHCAPVCC))
  {
    ESP_LOGE(TAG, "display main init fail");
    for (;;)
      ;
  }
  else
  {
    ESP_LOGI(TAG, "displa main init success");
  }

  ESP_LOGI(TAG, "Init display Sub");
  displaySub = Adafruit_SSD1306(128, 32, &Wire, -1);
  if (!displaySub.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    ESP_LOGE(TAG, "display sub init fail");
    for (;;)
      ;
  }
  else
  {
    ESP_LOGI(TAG, "display sub init success");
  }

  displayMain.display();
  displaySub.display();
  displayMain.cp437(true);
  displaySub.cp437(true);
  displayMain.setTextSize(2);
  displaySub.setTextSize(2);
  displaySub.setTextColor(SSD1306_WHITE);
  displayMain.setTextColor(SSD1306_WHITE);

  ESP_LOGI(TAG, "Set display update task");
  dataAndDisplay mainData;
  mainData.display = &displayMain;
  mainData.radio = radio;
  xTaskCreate(TaskDisplayMain,
              "display main",
              2048,
              (void *)&mainData,
              2, NULL);

  dataAndDisplay subData;
  subData.display = &displaySub;
  subData.radio = radio;
  xTaskCreate(TaskDisplaySub,
              "display sub",
              4096,
              (void *)&subData,
              2, NULL);
}