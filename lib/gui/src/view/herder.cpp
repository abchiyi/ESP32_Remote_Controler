#include "WouoUI.h"
#include "power.h"
void render_header(BasePage *page)
{
  auto screen_width = page->gui->DISPLAY_WIDTH;
  auto u8g2 = page->u8g2;

  u8g2->setDrawColor(1); // 点亮区域所有像素
  u8g2->drawBox(0, 0, screen_width, 20);
  u8g2->setDrawColor(2); // 全部切换为黑色
  u8g2->drawBox(0, 0, screen_width, 20);
  u8g2->setCursor(0, 10);
  u8g2->drawLine(0, 16, screen_width, 16);
  // u8g2->printf("Battery %d % ", PMU.getBatteryPercent());
  page->draw_slider_y(PMU.getBatteryPercent() / 100.0f, 0, 0, 8, 15);
};