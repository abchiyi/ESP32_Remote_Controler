/* 窗口实际是切换到了其他的页面，并将唤起窗口的页面指针作为参数接收在窗口的下方绘制该页面作为背景以达到看起来窗口是浮动在其他页面之上的效果 */

#include <WouoUI.h>
#include <U8g2lib.h>

// 弹窗变量
#define WIN_FONT u8g2_font_HelvetiPixel_tr
#define WIN_H 27
#define WIN_W 90
#define WIN_TITLE_W 20
#define WIN_TITLE_H 8
#define WIN_TITLE_S 5
#define WIN_VALUE_S 67
#define WIN_BAR_H 3

void popWindow(const char[], uint8_t *, uint8_t, uint8_t, uint8_t, BasePage *);

class C_WINDOW : public BasePage
{
private:
  char title[WIN_TITLE_W];
  uint8_t *value;
  uint8_t max;
  uint8_t min;
  uint8_t step;
  BasePage *bg_page;

  bool init_flag = false;
  bool exit_flag = false;
  bool oper_flag = false;

  float box_x;

  float box_y;
  float box_y_trg;

  float box_w;
  float box_w_trg;

  float box_H;
  float box_h;
  float box_h_trg;

  float bar_x;
  float bar_x_trg;

public:
  page_name_t name = "pop window";
  void create() {}; // 创建页面，在页面初始化时被调用

  friend void popWindow(const char[], uint8_t *, uint8_t, uint8_t, uint8_t, BasePage *);

  void render()
  {
    // 在进场时更新的参数
    if (!this->init_flag)
    {
      this->init_flag = true;
      this->oper_flag = true;
      this->box_y = 0;
      this->box_y_trg = (gui->DISPLAY_HEIGHT - WIN_H) / 2;
      this->box_w = gui->DISPLAY_WIDTH;
      this->box_w_trg = WIN_W;
      this->box_H = WIN_H;
      this->box_h = 0;
      this->box_h_trg = this->box_H + CONFIG_UI[WIN_Y_OS];
      this->bar_x = 0;

      u8g2->setFont(WIN_FONT);
    }

    // 在离场时更新的参数
    if (this->exit_flag)
    {
      this->box_H = 0;
      this->box_y_trg = 0;
      this->box_w_trg = gui->DISPLAY_WIDTH;
      if (!this->box_y)
      {
        gui->pageSwitch(this->bg_page);
        this->init_flag = false;
        this->exit_flag = false;
      }
    }

    // 在每次操作后都会更新的参数
    if (this->oper_flag)
    {
      this->oper_flag = false;
      this->bar_x_trg = (float)(*this->value - this->min) / (float)(this->max - this->min) * (this->box_w_trg - 2 * WIN_TITLE_S);
    }

    auto winAni = CONFIG_UI[WIN_ANI];
    // 计算动画过渡值
    animation(&this->box_y, &this->box_y_trg, winAni);
    animation(&this->box_w, &this->box_w_trg, winAni);
    animation(&this->box_h, &this->box_h_trg, winAni);
    animation(&this->box_h_trg, &this->box_H, winAni);
    animation(&this->bar_x, &this->bar_x_trg, winAni);

    // 绘制背景列表和窗口
    // gui->list_show(this->bg_index, this->bg);
    this->bg_page->render();
    this->box_x = (gui->DISPLAY_WIDTH - this->box_w) / 2;
    u8g2->setDrawColor(0);
    u8g2->drawBox(this->box_x, this->box_y, this->box_w, this->box_h); // 绘制外框背景
    u8g2->setDrawColor(2);
    u8g2->drawFrame(this->box_x, this->box_y, this->box_w, this->box_h); // 绘制外框描边
    if (this->box_h > (WIN_TITLE_H + 2 * WIN_TITLE_S))
    {
      u8g2->setCursor(this->box_x + WIN_VALUE_S, this->box_y + WIN_TITLE_S + WIN_TITLE_H);
      u8g2->print(*this->value); // 绘制数值
      u8g2->setCursor(this->box_x + WIN_TITLE_S, this->box_y + WIN_TITLE_S + WIN_TITLE_H);
      u8g2->print(this->title);                                                                                                  // 绘制标题
      u8g2->drawBox(this->box_x + WIN_TITLE_S, this->box_y + this->box_h - WIN_TITLE_S - WIN_BAR_H - 1, this->bar_x, WIN_BAR_H); // 绘制进度条
    }
  }

  void onUserInput(int8_t btnID)
  {
    if (this->box_y != this->box_y_trg && this->box_y_trg == 0)
      return;

    this->oper_flag = true;
    switch (btnID)
    {
    case BTN_ID_UP:
      if (*this->value < this->max)
        *this->value += this->step;
      // eeprom.change = true; // XXX EEPOM 不能储存更改后的值
      break;
    case BTN_ID_DO:
      if (*this->value > this->min)
        *this->value -= this->step;
      // eeprom.change = true;
      break;
    case BTN_ID_CANCEL:
    case BTN_ID_CONFIRM:
      this->exit_flag = true;
      break;
    }
  }
};

extern C_WINDOW *P_WINDOW;
