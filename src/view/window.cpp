#include <view/window.h>

C_WINDOW *P_WINDOW = new C_WINDOW;

/*
 * @brief 弹出数值更改窗口
 * @param *gui WouoUI对象指针
 * @param title 窗口标题
 * @param *value 要修改的值的指针
 * @param max 最大值
 * @param min 最小值
 * @param step 步长
 * @param *page BasePage或其子类实例指针。背景页面， 通常使用唤起窗口的页面作为背景
 */
void popWindow(const char title[], uint8_t *value, uint8_t max, uint8_t min, uint8_t step, BasePage *page)
{
  // ESP_LOGI(this->name, "window index %d", this->index);

  P_WINDOW->gui->pageSwitch(P_WINDOW);
  strcpy(P_WINDOW->title, title);
  P_WINDOW->value = value;
  P_WINDOW->max = max;
  P_WINDOW->min = min;
  P_WINDOW->step = step;
  P_WINDOW->bg_page = page;
}
