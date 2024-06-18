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
#include <storage_config.h>
#include <cstdlib> // 或者 #include <stdlib.h>

#define TAG "WouUI"

BOX BasePage::CURSOR; // 声明静态 box 对象;

float ListPage::text_y;
float ListPage::text_x;
float ListPage::text_x_trg;
float ListPage::text_y_trg;

uint8_t CONFIG_UI[UI_PARAM] = {
    255, // DISP_BRI
    15,  // BOX_X_OS
    10,  // BOX_Y_OS
    30,  // WIN_Y_OS
    120, // LIST_ANI
    60,  // WIN_ANI
    30,  // FADE_ANI
    20,  // BTN_SPT
    200, // BTN_LPT
    0    // COME_SCR
};

// GUI 基本参数
ConfigHandle<uint8_t[11]> config_ui = create_sconfig(CONFIG_UI);

void cb_fn_ui(bool mode)
{
  STORAGE_CONFIG.RW(config_ui, mode);
};

view_cb_t create_page_jump_fn(page_jump_mode_t mode, BasePage *&page)
{
  return [&, mode](WouoUI *ui)
  {
    mode == PAGE_IN ? ui->page_in_to(page) : ui->page_out_to(page);
  };
};

/* 以进入的形式前往指定页面 */
void WouoUI::page_in_to(BasePage *page)
{
  this->index_targe = page->index;
  this->state = STATE_LAYER_IN;
}

/* 以退出的形式前往指定页面 */
void WouoUI::page_out_to(BasePage *page)
{
  this->index_targe = page->index;
  this->state = STATE_LAYER_OUT;
}

/* 以切换形式前往页面 */
void WouoUI::pageSwitch(BasePage *page)
{
  this->index = page->index;
  this->state = STATE_VIEW;
}

// 进入更深层级时的初始化
void WouoUI::layer_in()
{
  this->state = STATE_FADE;
  this->init_flag = false;

  auto page = this->getPage();                   // 当前页面
  auto page_target = this->getPage(index_targe); // 目标页面

  page_target->select = 0;
  page_target->cursor_position_y = 0;

  auto calc = [&](uint8_t v)
  { return v * (page->cursor_position_y / LIST_LINE_H); };

  // XXX 同质代码合并
  page->CURSOR.width += calc(config_ui.ref[BOX_X_OS]);
  page->CURSOR.height += calc(config_ui.ref[BOX_Y_OS]);

  this->index = this->index_targe;
}

// 进入更浅层级时的初始化
void WouoUI::layer_out()
{
  auto page = this->getPage();                   // 当前页面
  auto page_target = this->getPage(index_targe); // 目标页面

  this->state = STATE_FADE;
  this->init_flag = false;

  auto calc = [&](uint8_t v)
  {
    return v * abs((page_target->cursor_position_y - page->cursor_position_y) / LIST_LINE_H);
  };

  // XXX 同质代码合并
  page_target->CURSOR.width += calc(config_ui.ref[BOX_X_OS]);
  page_target->CURSOR.height += calc(config_ui.ref[BOX_Y_OS]);

  page->leave();
  this->index = this->index_targe;
}

/************************************* 动画函数 *************************************/

// 消失动画
void WouoUI::fade()
{
  static uint8_t fade_ani;
  static uint8_t fade = 1;

  if (fade == 1)
  {
    config_ui.access([&]()
                     { fade_ani = config_ui.ref[FADE_ANI]; });
  }
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
    this->state = STATE_VIEW;
    fade = 0;
    break;
  }
  fade++;
}

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

// OLED初始化函数
void WouoUI::oled_init()
{
  this->u8g2->setBusClock(10000000);
  this->u8g2->begin();
  this->u8g2->enableUTF8Print();

  this->buf_ptr = this->u8g2->getBufferPtr();
  this->buf_len =
      8 * this->u8g2->getBufferTileHeight() * this->u8g2->getBufferTileWidth();
  config_ui.access([&]()
                   { this->u8g2->setContrast(config_ui.ref[DISP_BRI]); });
}

/*
 * @brief 添加对象式页面
 * @param page 页面指针
 */
void WouoUI::addPage(BasePage *page)
{
  static uint8_t pageIndex = 0;
  page->u8g2 = this->u8g2;           // u8g2 指针
  page->gui = this;                  // 设置gui引用
  pageIndex++;                       // 页码+1
  page->index = pageIndex;           // 设置页码
  page->create();                    // 初始化页面
  this->objPage[page->index] = page; // 储存页面
};

// 设置默认页面
void WouoUI::setDefaultPage(BasePage *page)
{
  this->index = page->index;
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
  case STATE_VIEW:
    auto page = this->getPage();
    this->u8g2->clearBuffer();
    if (!this->init_flag)
    {
      this->init_flag = true;
      page->before();
    }
    page->render();
  }
}

void WouoUI::begin(U8G2 *u8g2)
{
  // 设置屏幕指针
  this->u8g2 = u8g2;
  // 获取屏幕宽高
  this->DISPLAY_WIDTH = u8g2->getWidth();
  this->DISPLAY_HEIGHT = u8g2->getHeight();

  this->oled_init();

  // 设置屏幕刷新任务
  auto taskUpdate = [](void *pt)
  {
    const TickType_t xFreequency = 10; // 渲染一帧间隔 /ms
    TickType_t xLastWakeTime = xTaskGetTickCount();
    WouoUI *gui = (WouoUI *)pt;
    while (true)
    {
      gui->uiUpdate();
      vTaskDelayUntil(&xLastWakeTime, xFreequency);
    }
  };
  xTaskCreatePinnedToCore(taskUpdate, "WouoUI gui update", 1024 * 10, (void *)this, 1, NULL, 1);
}

/*
 * @brief 按键事件更新，建议与渲染更新使用不同的任务执行以避免阻塞视图渲染
 * @param func 按钮扫描函数，在函数内自定义要执行的操作
 */
void WouoUI::btnUpdate(void (*func)(WouoUI *))
{
  func(this);           // 按钮扫描函数
  if (this->btnPressed) // 当有按键按下时触发input函数
  {
    this->btnPressed = false;
    this->getPage()->onUserInput(this->btnID);
  }
}

/* ----------------- Base Page ----------------- */

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

/* ----------------- List Page ----------------- */
// 处理按钮事件
void ListPage::onUserInput(int8_t btnID)
{
  gui->oper_flag = true;
  switch (btnID)
  {
  case BTN_ID_UP:
    ESP_LOGD(TAG, "MOVE_UP");
    cursorMoveUP(); // 光标向上移动一行
    break;

  case BTN_ID_DO:
    ESP_LOGD(TAG, "MOVE_DOWN");
    cursorMoveDOWN(); // 光标向上移动一行
    break;

  case BTN_ID_CANCEL: // 返回
    ESP_LOGD(TAG, "CANCEL");
    select = 0;
  case BTN_ID_CONFIRM: // 确认
    ESP_LOGD(TAG, "CONFIRM");
    // 执行与 view uint 绑定的回调函数，通常是页面跳转
    if (this->view[select].cb_fn)
      this->view[select].cb_fn(gui);
    setCursorOS(config_ui.ref[BOX_X_OS]);
    break;
  }
}

// 列表页面渲染函数
void ListPage::render()
{
  auto length = this->view.size();
  auto page = gui->getPage();

  static int16_t text_x_temp; // 文本横轴起始坐标

  static uint8_t list_ani;
  static uint8_t com_scr;

  config_ui.access([&]()
                   {
                     list_ani = config_ui.ref[LIST_ANI];
                     com_scr = config_ui.ref[COME_SCR]; });

  // 在每次操作后都会更新的参数
  // if (gui->oper_flag)
  // {
  //   gui->oper_flag = false;

  // }

  // 绘制光标
  auto render_cursor = [&]()
  {
    /**
     * 检查光标位置是否正确
     * 当光标越界时，移动到列表的末尾
     */
    auto viewSize = view.size() - 1;
    if (select > viewSize && viewSize != 0)
      cursorMoveUP(select - viewSize);

    CURSOR.min_width =
        u8g2->getUTF8Width(view[select].m_select.c_str()) + LIST_TEXT_S * 2;
    this->draw_cursor();
  };

  auto render_list = [&]()
  {
    animation(&text_x, 0.0f, list_ani);
    animation(&text_y, &text_y_trg, list_ani);

    /**
     * 绘制末尾元素
     * 文本第一个字符作为绘制模式控制符号
     * '~' 末尾绘制变量
     * '+' 末尾绘多选框
     * '-' 不绘制元素
     * '=' 末尾绘制单选框
     */
    auto render_end_element = [&](uint index)
    {
      switch (view[index].m_select[0])
      {
      case '~':
        // list_draw_val(index - 1);
        break;
      case '+':
        break;
      case '=':
        break;
      }
    };

    // 绘制列表文字和行末尾元素
    for (int i = 0; i < length; ++i)
    {
      // 绘制文本
      text_y_temp = text_y + LIST_LINE_H * i;
      text_x_temp = text_x * (!com_scr ? (abs(gui->getPage()->select - i) + 1)
                                       : (i + 1));
      text_w_temp = text_x_temp + LIST_TEXT_W;

      u8g2->setCursor(text_x_temp + LIST_TEXT_S, LIST_TEXT_S + LIST_TEXT_H + text_y_temp);
      u8g2->print(view[i].m_select.c_str());
      if (view[i].render_end)
        view[i].render_end(gui);
      // render_end_element(i);
    }
  };

  auto render_bar = [&]()
  {
    this->bar_h_trg = ceil(gui->getPage()->select *
                           ((float)gui->DISPLAY_HEIGHT / (length - 1)));
    animation(&this->bar_h, &this->bar_h_trg, list_ani);
    u8g2->drawBox(gui->DISPLAY_WIDTH - LIST_BAR_W, 0, LIST_BAR_W, this->bar_h);
  };

  render_bar();
  render_list();
  render_cursor();
};

void ListPage::before()
{
  gui->oper_flag = true;
  text_x = -gui->DISPLAY_WIDTH;
  text_y = cursor_position_y - LIST_LINE_H * select;
  text_y_trg = text_y;

  // CURSOR
  CURSOR.x = 0;
  CURSOR.round_corner = 0.5;
  CURSOR.min_height = LIST_LINE_H;
  CURSOR.transition = config_ui.ref[LIST_ANI];

  u8g2->setFont(LIST_FONT);
  u8g2->setDrawColor(2);
};

void ListPage::cursorMoveDOWN(uint step)
{
  uint8_t box_x_os = config_ui.ref[BOX_X_OS];

  auto move = [&]()
  {
    if (select != (((ListPage *)gui->getPage())->view.size() - 1))
    {
      setCursorOS(box_x_os, config_ui.ref[BOX_Y_OS]); // 光标轮廓扩大
      select += 1;                                    // 选中行数下移一位
      // 光标到达屏幕底部
      cursor_position_y >= (gui->DISPLAY_HEIGHT - LIST_LINE_H)
          ? text_y_trg -= LIST_LINE_H         // 上翻列表
          : cursor_position_y += LIST_LINE_H; // 下移光标
    }
    else
      setCursorOS(box_x_os); // 光标横向扩大

    ESP_LOGD("TAG", "step %d, select %d, pos_Y %.2f text_y %.2f",
             step, select, cursor_position_y, text_y_trg);
  };

  if (step < 2)
    move();
  else
    for (size_t i = 0; i < step; i++)
      move();
};

void ListPage::cursorMoveUP(uint step)
{
  uint8_t box_x_os = config_ui.ref[BOX_X_OS];

  auto move = [&]()
  {
    if (select != 0)
    {
      setCursorOS(box_x_os, config_ui.ref[BOX_Y_OS]); // 光标轮廓扩大
      select -= 1;                                    // 选中行数上移一位
      cursor_position_y                               // 当光标未到达屏幕顶部时
          ? cursor_position_y -= LIST_LINE_H          // 上移光标
          : text_y_trg += LIST_LINE_H;                // 下翻列表
    }
    else
      setCursorOS(box_x_os); // 光标横向扩大

    ESP_LOGD("TAG", "step %d, select %d, pos_Y %.2f text_y %.2f",
             step, select, cursor_position_y, text_y_trg);
  };

  if (step < 2)
    move();
  else
    for (size_t i = 0; i < step; i++)
      move();
};