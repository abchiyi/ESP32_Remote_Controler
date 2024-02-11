#include <display.h>
#include <esp_log.h>

#define TAG "Display"

TimerHandle_t conntingAnimation;
int conntingAnimationID = 0;
bool conntingAnimationTimerStared = false;

struct dataAndDisplay
{
  Adafruit_SSD1306 *display;
  Radio *radio;
};

void TaskDisplayMain(void *pt)
{
  ESP_LOGI(TAG, "set data");
  auto [display, radio] = *(dataAndDisplay *)pt;

  ESP_LOGI(TAG, "start while");
  while (true)
  {
    display->clearDisplay();
    display->setTextSize(1);
    // 电池电压
    display->setCursor(0, 0);
    display->printf("Battery : %.1f V", radio->RecvData.volts);
    display->display();
    vTaskDelay(16);
  }
}

// 连接加载动画
int connting = 0;

void conntingAnimationCB(TimerHandle_t xTimer)
{
  connting++;
  if (connting == 4)
    connting = 0;
  // ESP_LOGW(TAG, "connting %d", connting);
}

// 副屏幕显示内容
void TaskDisplaySub(void *pt)
{
  ESP_LOGI(TAG, "display get data");
  auto [display, radio] = *(dataAndDisplay *)pt;
  ESP_LOGI(TAG, "display start update");

  while (true)
  {
    display->clearDisplay();

    // display->write(radio->isPaired ? 0x12 : 0x18);

    if (!radio->isPaired && !conntingAnimationTimerStared)
    {
      ESP_LOGI(TAG, "scan anm timer start");
      xTimerStart(conntingAnimation, 300);
      conntingAnimationTimerStared = true;
    }

    if (radio->isPaired && conntingAnimationTimerStared)
    {
      ESP_LOGI(TAG, "scan anm timer stop");
      xTimerStop(conntingAnimation, 300);
      conntingAnimationTimerStared = false;
    }

    if (radio->isPaired)
    {
      display->setTextSize(3);
      display->setCursor(0, 0);
      display->write(0x12);

      display->setTextSize(1);
      display->setCursor(0, 24);
      display->write("connted");
    }
    else
    {
      switch (connting)
      {
      case 0:
        display->setTextSize(3);
        display->setCursor(0, 0);
        display->write(0x2d);

        display->setTextSize(1);
        display->setCursor(0, 24);
        display->write("Scanning");
        break;
      case 1:
        display->setTextSize(3);
        display->setCursor(0, 0);
        display->write(0x5c);

        display->setTextSize(1);
        display->setCursor(0, 24);
        display->write("Scanning.");
        break;
      case 2:
        display->setTextSize(3);
        display->setCursor(0, 0);
        display->write(0xb3);

        display->setTextSize(1);
        display->setCursor(0, 24);
        display->write("Scanning..");
        break;
      case 3:
        display->setTextSize(3);
        display->setCursor(0, 0);
        display->write(0x2f);

        display->setTextSize(1);
        display->setCursor(0, 24);
        display->write("Scanning...");
        break;
      }
    }

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

  conntingAnimation = xTimerCreate(
      "Connect time out",           // 定时器任务名称
      150,                          // 延迟多少tick后执行回调函数
      pdTRUE,                       // 执行一次,pdTRUE 循环执行
      (void *)&conntingAnimationID, // 任务id
      conntingAnimationCB           // 回调函数
  );

  dataAndDisplay subData;
  subData.display = &displaySub;
  subData.radio = radio;
  xTaskCreate(TaskDisplaySub,
              "display sub",
              4096,
              (void *)&subData,
              2, NULL);

  ESP_LOGI(TAG, "init success");
}