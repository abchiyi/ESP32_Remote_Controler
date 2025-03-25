/*
  此项目为 WouoUI 的精简和优化版本，又名 WouoUI - Lite_General_Pro_Plus_Ultra

  与 WouoUI - Lite_General 相比，减少的功能和特性：

    * 主题切换：去除了白色主题遮罩
    * 列表循环：去除了选择框在头尾继续操作时，跳转至另一头的功能
    * 任意行高：去除了支持屏幕高度与行高度不能整除的特性，这个版本需要能够整除
    * 背景虚化：去除了弹窗背景虚化的可选项

  与 WouoUI - Lite_General 相比，增加和有区别的功能和特性有：

    * 展开列表动画从Y轴展开改为X轴排队进入
    * 进入更深层级时，选择框从头展开改为跳转
    * 增加选择时拖影，选择滚动越快拖影越长，XY轴都有拖影
    * 增加跳转时拖影，选择跳转越远拖影越长，XY轴都有拖影
    * 撞墙特性：滚动时当后续还有选择项时，有Y轴拖影，没有时没有Y轴拖影

  此项目使用的开源协议为GPL，以下简单解释权利和义务，详情请参考文档

    权利：

    * 不限制数量的复制
    * 不限制方式的传播
    * 允许增加，删除和修改代码
    * 允许收费，但以服务的形式

    义务：

    * 新项目也要开源，不能通过代码本身盈利
    * 新项目也要使用该许可证
    * 出售时要让买家知道可以免费获得

  请保留以下该信息以大致遵守开源协议

    * 作者：B站用户名：音游玩的人
    * 开源：https://github.com/RQNG/WoW-UI
*/
#include <SPI.h>
#include <WouoUI.h>
#include <cstdlib>
#include <esp_log.h>
#include <config.h>
#include <tool.h>

#define TAG "WouUI"
constexpr TickType_t FPS = HZ2TICKS(60); // 屏幕刷新率
WouoUI WOUO_UI;                          // 定义gui对象
BOX BasePage::CURSOR;                    // 声明静态 box 对象;

uint8_t CONFIG_UI[UI_PARAM] = {
    255, // DISP_BRI
    15,  // BOX_X_OS
    10,  // BOX_Y_OS
    30,  // WIN_Y_OS
    80,  // LIST_ANI
    40,  // WIN_ANI
    20,  // FADE_ANI
    20,  // BTN_SPT
    200, // BTN_LPT
    0    // COME_SCR
};

gui_cb_fn_t create_page_jump_fn(create_page_fn_t cb_fn)
{
  return [=](WouoUI *ui)
  {
    cb_fn
        ? ui->page_in_to(cb_fn)
        : ui->page_back();
  };
};

// 渐近动画
void animation(float *a, float *a_trg, uint8_t n)
{
  if (!n)
    *a = *a_trg;

  if (*a != *a_trg)
  {
    if (fabs(*a - *a_trg) < 0.15f)
      *a = *a_trg;
    else
      *a += (*a_trg - *a) / (n / 10.0f);
  }
}

void animation(float *a, float a_trg, uint8_t n)
{
  animation(a, &a_trg, n);
}

/* ----------------- Wouo UI ----------------- */

void WouoUI::page_in_to(create_page_fn_t cb_fn)
{
  auto pt_page = cb_fn();
  pt_page->u8g2 = this->u8g2; // u8g2 指针
  pt_page->gui = this;        // 设置gui引用
  this->history.push_back(pt_page);
  this->state = STATE_LAYER_IN;
  pt_page->create(); // 初始化页面
};

void WouoUI::page_back()
{
  if (history.size() > 1)
    this->state = STATE_LAYER_OUT;
}

/* 以切换形式前往页面 */
void WouoUI::pageSwitch(BasePage *page)
{
  this->state = STATE_VIEW;
}

// 进入更深层级时的初始化
void WouoUI::layer_in()
{
  ESP_LOGD(TAG, "PAGE IN");
  this->state = STATE_FADE;

  auto page_before = this->get_history(-2); // 上一个页面
  auto page = this->get_history();          // 当前页面
  page->select = 0;
  page->cursor_position_y = 0;
  auto calc = [&](uint8_t v) -> float
  {
    if (!page_before->cursor_position_y)
      return 0.0;
    return v * (page_before->cursor_position_y / LIST_LINE_H);
  };

  page_before->setCursorOS(calc(CONFIG_UI[BOX_X_OS]),
                           calc(CONFIG_UI[BOX_Y_OS]));
}

// 进入更浅层级时的初始化
void WouoUI::layer_out()
{
  ESP_LOGD(TAG, "PAGE OUT");
  auto page_before = this->get_history(-2);
  auto page = this->get_history();

  this->state = STATE_FADE;

  auto calc = [&](uint8_t v)
  {
    return v * abs((page->cursor_position_y - page_before->cursor_position_y) / LIST_LINE_H);
  };
  page->setCursorOS(calc(CONFIG_UI[BOX_X_OS]),
                    calc(CONFIG_UI[BOX_Y_OS]));
  page->leave();
  this->history.pop_back(); // 销毁历史
  delete page;              // 销毁页面
}

BasePage *WouoUI::get_history(int index)
{
  auto size = history.size();
  if (!size)
    esp_system_abort("History is empty");

  if (index == -1 || size >= index || size < abs(index))
    return this->history.back().page;

  if (index < 0)
    return this->history[(size + index)].page;
  else
    return this->history[index].page;
};

// 消失动画
void WouoUI::fade()
{
  static uint8_t fade_ani;
  static uint8_t fade = 1;

  if (fade == 1)
    fade_ani = CONFIG_UI[FADE_ANI];

  delay(fade_ani);

  switch (fade)
  {
  case 1:
    for (uint16_t i = 0; i < this->buf_len; ++i)
      if (i % 2 != 0)
        this->buf_ptr[i] = this->buf_ptr[i] & 0xAA;
    break;
  case 2:
    for (uint16_t i = 0; i < this->buf_len; ++i)
      if (i % 2 != 0)
        this->buf_ptr[i] = this->buf_ptr[i] & 0x00;
    break;
  case 3:
    for (uint16_t i = 0; i < this->buf_len; ++i)
      if (i % 2 == 0)
        this->buf_ptr[i] = this->buf_ptr[i] & 0x55;
    break;
  case 4:
    for (uint16_t i = 0; i < this->buf_len; ++i)
      if (i % 2 == 0)
        this->buf_ptr[i] = this->buf_ptr[i] & 0x00;
    break;
  default:
    this->state = STATE_BEFORE_VIEW;
    fade = 0;
    break;
  }
  fade++;
}

void WouoUI::gui_sleep()
{
  this->u8g2->setPowerSave(1);            // 关闭屏幕
  xTimerStop(this->xTimer_GUI_SLEEP, 10); // 停止定时器
};

void WouoUI::gui_awake()
{
  u8g2->setPowerSave(0);                   // 开启屏幕
  this->page_back();                       // 退出睡眠页
  xTimerStart(this->xTimer_GUI_SLEEP, 10); // 重置定时器
};

// OLED初始化函数
void WouoUI::oled_init()
{
  this->u8g2->setBusClock(10000000);
  this->u8g2->begin();
  this->u8g2->enableUTF8Print();

  this->buf_ptr = this->u8g2->getBufferPtr();
  this->buf_len =
      8 * this->u8g2->getBufferTileHeight() * this->u8g2->getBufferTileWidth();
  this->u8g2->setContrast(CONFIG_UI[DISP_BRI]);

  this->DISPLAY_WIDTH = u8g2->getWidth();
  this->DISPLAY_HEIGHT = u8g2->getHeight();
}

// 设置默认页面
void WouoUI::setDefaultPage(create_page_fn_t cb_fn)
{
  if (history.size() < 1) // index 0 为默认页面
    page_in_to(cb_fn);
}

// 总进程
void WouoUI::uiUpdate()
{
  this->u8g2->sendBuffer();
  switch (this->state)
  {
  case STATE_LAYER_IN:
    layer_in();
    break;
  case STATE_LAYER_OUT:
    layer_out();
    break;
  case STATE_FADE:
    this->fade();
    break;
  case STATE_BEFORE_VIEW:
    ESP_LOGI(TAG, "before view");
    this->get_history()->before();
    this->state = STATE_VIEW;
    break;
  case STATE_VIEW: // 时刻刷新页面
    this->u8g2->clearBuffer();
    auto page = this->get_history();
    // 渲染页面&窗口
    page->__base_render();
    for (auto window : page->windows)
      window->render();

    // 处理按键事件
    event_t event;
    if (xQueueReceive(Q_Event, &event, 2) == pdTRUE)
    {
      ESP_LOGI(TAG, "event: %d", event.key_id);
      if (page->windows.size()) // 当有窗口显示时，顶部窗口接管按键
        get_top_window()->onUserInput(event);
      else
        for (auto on : page->eventListeners)
          if (event.key_id == on.key)
            on.cb_fn();
    }
  }
}

// 睡眠定时器回调
#include "view/sleep.h"
void sleep_cb(TimerHandle_t timer)
{
  ESP_LOGI(TAG, "Sleep");
  WOUO_UI.page_in_to(page_sleep);
};

void WouoUI::begin(U8G2 *u8g2, create_page_fn_t home_page)
{
  // 初始化显示屏
  this->u8g2 = u8g2;
  this->oled_init();

  // 设置主页面
  this->setDefaultPage(home_page);

  // 配置休眠定时器
  xTimer_GUI_SLEEP = xTimerCreate(
      "timer screen sleep", // 定时器名称
      // TODO 再GUI变量中定义休眠延迟时间
      pdMS_TO_TICKS(60000 * 10), // 定时器周期（以系统节拍为单位）
      pdTRUE,                    // pdTrue自动重载（周期性定时器）
      (void *)0,                 // 定时器ID
      sleep_cb                   // 回调函数
  );

  // if (xTimer_GUI_SLEEP == NULL)
  // {
  //   ESP_LOGE(TAG, "xTimer_GUI_SLEEP creation failed!");
  //   for (;;) //  报告错误并停止程序
  //     ;
  // }
  // else
  //   xTimerStart(xTimer_GUI_SLEEP, 10);

  // 设置屏幕刷新任务
  auto taskUpdate = [](void *pt)
  {
    static TickType_t xLastWakeTime = xTaskGetTickCount();
    static WouoUI *gui = (WouoUI *)pt;
    while (true)
    {
      gui->uiUpdate();
      vTaskDelayUntil(&xLastWakeTime, FPS);
    }
  };

  xTaskCreate(taskUpdate, "taskUpdate", 4096, this, TP_H, NULL);
};

#include "view/herder.h"
/* ----------------- Base Page ----------------- */
void BasePage::__base_render()
{
  this->render();
  // render_header(this);
};

void BasePage::draw_slider_y(float progress,
                             uint8_t x, uint8_t y, uint8_t width,
                             uint8_t length, bool biaxial)
{

  uint8_t padding_bottom = y + length - 1;
  uint8_t padding_left = x + 1;
  uint8_t padding_top = y + 1;
  uint8_t box_width = width - 2;

  uint8_t line_pos_y = y + length / 2; // 中位线y轴坐标
  uint8_t line_end = padding_left + box_width - 1;
  uint8_t line_start = padding_left;

  auto box_y = biaxial
                   ? (progress > 0 ? line_pos_y : line_pos_y + 1)
                   : (y + length - 1);

  /*----- 计算进度 -----*/

  // 计算要填充的最大像素长度
  auto pixel_length = biaxial
                          ? progress > 0
                                ? line_pos_y - padding_top
                                : padding_bottom - line_pos_y - 1
                          : length - 2;
  // 实际填充的像素长度
  uint8_t box_height = round(abs(progress * pixel_length));

  auto box_y_pos = (progress < 0 && biaxial) ? box_y : box_y - box_height;

  /*----- 绘制 -----*/

  u8g2->drawFrame(x, y, width, length);                          // 绘制外框
  u8g2->drawBox(padding_left, box_y_pos, box_width, box_height); // 绘制填充
  if (biaxial)                                                   // 绘制中位线
    u8g2->drawLine(line_start, line_pos_y, line_end, line_pos_y);
}

void BasePage::draw_slider_x(float progress,
                             uint8_t x, uint8_t y, uint8_t width,
                             uint8_t length, bool biaxial)
{
  uint8_t padding_right = x + length - 1;
  uint8_t padding_left = x + 1;
  uint8_t padding_top = y + 1;
  uint8_t box_height = width - 2;

  uint8_t line_pos_x = x + length / 2; // 中位线 x 轴坐标
  uint8_t line_end = padding_top + box_height - 1;
  uint8_t line_start = padding_top;

  auto box_x = biaxial
                   ? (progress < 0 ? line_pos_x : line_pos_x + 1)
                   : padding_left;

  // 计算要填充的最大像素长度
  auto pixel_length = biaxial
                          ? progress < 0
                                ? line_pos_x - padding_left
                                : padding_right - line_pos_x - 1
                          : length - 2;
  // 实际填充的像素长度
  uint8_t box_width = round(abs(progress * pixel_length));
  auto box_x_pos = (progress < 0 && biaxial) ? box_x - box_width : box_x;

  u8g2->drawBox(box_x_pos, padding_top, box_width, box_height);
  u8g2->drawFrame(x, y, length, width); // 绘制外框
  if (biaxial)                          // 绘制中位线
    u8g2->drawLine(line_pos_x, line_start, line_pos_x, line_end);
}

/* ----------------- Base Window ----------------- */
void BaseWindow::close_window()
{
  this->leave();
  this->gui->get_history()->windows.pop_back();
  delete this;
};

void BaseWindow::before()
{
  this->oper_flag = true;
  this->box_y = 0;
  this->box_y_trg = (gui->DISPLAY_HEIGHT - WIN_H) / 2;
  this->box_w = gui->DISPLAY_WIDTH;
  this->box_w_trg = WIN_W;
  this->box_H = WIN_H;
  this->box_h = 0;
  this->bar_x = 0;
  this->box_h_trg = this->box_H + CONFIG_UI[WIN_Y_OS];

  u8g2->setFont(WIN_FONT);
}

void BaseWindow::leave()
{
}

bool BaseWindow::render()
{

  // 在每次操作后都会更新的参数
  if (this->oper_flag)
  {
    this->oper_flag = false;
    // this->bar_x_trg = (float)(*this->value - this->min) / (float)(this->max - this->min) * (this->box_w_trg - 2 * WIN_TITLE_S);
  }

  uint8_t winAni = CONFIG_UI[WIN_ANI];
  // 计算动画过渡值
  animation(&this->box_y, &this->box_y_trg, winAni);
  animation(&this->box_w, &this->box_w_trg, winAni);
  animation(&this->box_h, &this->box_h_trg, winAni);
  animation(&this->box_h_trg, &this->box_H, winAni);
  animation(&this->bar_x, &this->bar_x_trg, winAni);

  // 窗口背景
  this->box_x = (gui->DISPLAY_WIDTH - this->box_w) / 2;

  // 绘制外框背景
  u8g2->setDrawColor(0);
  u8g2->drawBox(this->box_x, this->box_y, this->box_w, this->box_h);

  // 绘制外框描边
  u8g2->setDrawColor(2);
  u8g2->drawFrame(this->box_x, this->box_y, this->box_w, this->box_h);

  // 在离场时更新的参数
  if (this->exit_flag)
  {
    this->box_w_trg = gui->DISPLAY_WIDTH;
    this->box_y_trg = 0;
    this->box_H = 0;
    if (!this->box_y)
    {
      ESP_LOGI("test", "run close_window");
      close_window();
      return true;
    }
  }
  return false;
};

void BaseWindow::onUserInput(event_t event)
{
  if (this->box_y != this->box_y_trg && this->box_y_trg == 0)
    return;
  this->oper_flag = true;

  switch (event.key_id)
  {
  // case KEY_UP:
  //   if (*this->value < this->max)
  //     *this->value += this->step;
  //   break;
  // case KEY_DOWN:
  //   if (*this->value > this->min)
  //     *this->value -= this->step;
  //   break;
  case KEY_BACK:
  case KEY_CONFIRM:
    this->leave();
    // gui->pageSwitch(this->bg_page);

    this->exit_flag = true;
    break;
  }
};
