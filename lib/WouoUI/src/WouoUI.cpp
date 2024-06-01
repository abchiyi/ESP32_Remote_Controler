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

#define TAG "WouUI"

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

/*********************************** 定义列表内容 ***********************************/

M_SELECT edit_f0_menu[]{
    {"[ Edit Fidget Toy ]"},
    {"~ Box X OS"},
    {"~ Box Y OS"},
    {"~ Box Ani"},
};

/************************************ 初始化函数 ***********************************/

// 单选框初始化
void WouoUI::check_box_s_init(uint8_t *param, uint8_t *param_p)
{
  check_box.s = param;
  check_box.s_p = param_p;
}

// 多选框初始化
void WouoUI::check_box_m_init(uint8_t *param)
{
  check_box.m = param;
}

// 数值初始化
void WouoUI::check_box_v_init(uint8_t *param)
{
  check_box.v = param;
}

// 单选框处理函数
void WouoUI::check_box_s_select(uint8_t val, uint8_t pos)
{
  *check_box.s = val;
  *check_box.s_p = pos;
  // eeprom.change = true; // TODO EERPROM 失效
}

// 多选框处理函数
void WouoUI::check_box_m_select(uint8_t param)
{
  check_box.m[param] = !check_box.m[param];
  // eeprom.change = true;
}

/* layer+1 以进入的形式前往指定页面 */
void WouoUI::page_in_to(BasePage *page)
{
  this->ui.index = page->index;
  this->ui.state = STATE_LAYER_IN;
}

/* layer-1 以退出的形式前往指定页面 */
void WouoUI::page_out_to(BasePage *page)
{
  this->ui.index = page->index;
  this->ui.state = STATE_LAYER_OUT;
}

/* layer 不变 以切换形式前往页面 */
void WouoUI::pageSwitch(BasePage *page)
{
  this->ui.index = page->index;
  this->ui.state = STATE_VIEW;
}

/*
 * @brief 切换页面
 * @param page 页面指针
 * @param state 页面切换模式
 */
void WouoUI::pageChange(BasePage *page, uint8_t state)
{
  this->ui.index = page->index;
  this->ui.state = state;
}

// 进入更深层级时的初始化
void WouoUI::layer_in()
{
  this->ui.layer++;
  this->ui.state = STATE_FADE;
  this->ui.init_flag = false;
  this->ui.select[this->ui.layer] = 0;

  list.box_y_trg[this->ui.layer] = 0;

  auto layer = this->ui.layer;
  auto calc = [layer](uint8_t v)
  { return v * (list.box_y_trg[layer - 1] / LIST_LINE_H); };

  list.box_w_trg += calc(CONFIG_UI[BOX_X_OS]);
  list.box_h_trg += calc(CONFIG_UI[BOX_Y_OS]);
}

// 进入更浅层级时的初始化
void WouoUI::layer_out()
{
  this->ui.layer--;
  this->ui.state = STATE_FADE;
  this->ui.init_flag = false;

  auto layer = this->ui.layer;
  auto calc = [layer](uint8_t v)
  { return v * abs((list.box_y_trg[layer] - list.box_y_trg[layer + 1]) / LIST_LINE_H); };

  list.box_w_trg += calc(CONFIG_UI[BOX_X_OS]);
  list.box_h_trg += calc(CONFIG_UI[BOX_Y_OS]);
}

/************************************* 动画函数 *************************************/

// 消失动画
void WouoUI::fade()
{
  delay(CONFIG_UI[FADE_ANI]);
  switch (this->ui.fade)
  {
  case 1:
    for (uint16_t i = 0; i < this->ui.buf_len; ++i)
      if (i % 2 != 0)
        this->ui.buf_ptr[i] = this->ui.buf_ptr[i] & 0xAA;
    break;
  case 2:
    for (uint16_t i = 0; i < this->ui.buf_len; ++i)
      if (i % 2 != 0)
        this->ui.buf_ptr[i] = this->ui.buf_ptr[i] & 0x00;
    break;
  case 3:
    for (uint16_t i = 0; i < this->ui.buf_len; ++i)
      if (i % 2 == 0)
        this->ui.buf_ptr[i] = this->ui.buf_ptr[i] & 0x55;
    break;
  case 4:
    for (uint16_t i = 0; i < this->ui.buf_len; ++i)
      if (i % 2 == 0)
        this->ui.buf_ptr[i] = this->ui.buf_ptr[i] & 0x00;
    break;
  default:
    this->ui.state = STATE_VIEW;
    this->ui.fade = 0;
    break;
  }
  this->ui.fade++;
}

// 渐近动画
void animation(float *a, float *a_trg, uint8_t n)
{
  if (*a != *a_trg)
  {
    if (fabs(*a - *a_trg) < 0.15f)
      *a = *a_trg;
    else
      *a += (*a_trg - *a) / (n / 10.0f);
  }
}

// 渐近动画
void animation(float *a, float a_trg, uint8_t n)
{
  animation(a, &a_trg, n);
}

/************************************* 显示函数 *************************************/
// // 编辑解压玩具菜单
// void edit_f0_proc()
// {
//   switch (ui.select[ui.layer])
//   {
//   case 0:
//     ui.index = M_EDITOR;
//     ui.state = STATE_S_LAYER_OUT;
//     break;
//   case 1:
//     win_init("F0 X OS", &f0.param[F0_X_OS], 100, 0, 1, ui.index, edit_f0_menu);
//     break;
//   case 2:
//     win_init("F0 Y OS", &f0.param[F0_Y_OS], 100, 0, 1, ui.index, edit_f0_menu);
//     break;
//   case 3:
//     win_init("F0 Ani", &f0.param[F0_ANI], 255, 0, 1, ui.index, edit_f0_menu);
//     break;
//   }
// }

// OLED初始化函数
void WouoUI::oled_init()
{
  this->u8g2->setBusClock(10000000);
  this->u8g2->begin();
  this->u8g2->enableUTF8Print();
  this->u8g2->setContrast(CONFIG_UI[DISP_BRI]);
  this->ui.buf_ptr = this->u8g2->getBufferPtr();
  this->ui.buf_len = 8 * this->u8g2->getBufferTileHeight() * this->u8g2->getBufferTileWidth();
}

static uint8_t pageIndex = 0;

/*
 * @brief 添加对象式页面
 * @param page 页面指针
 */
void WouoUI::addPage(BasePage *page)
{
  page->u8g2 = this->u8g2;              // u8g2 指针
  page->gui = this;                     // 设置gui引用
  pageIndex++;                          // 页码+1
  page->index = pageIndex;              // 设置页码
  page->create();                       // 初始化页面
  this->ui.objPage[page->index] = page; // 储存页面指针
};

// 设置默认页面
void WouoUI::setDefaultPage(BasePage *page)
{
  this->ui.index = page->index;
}

// 总进程
void WouoUI::uiUpdate()
{
  int8_t ui_lenght = ((ListPage *)this->ui.objPage[this->ui.index])->length;
  this->u8g2->sendBuffer();
  // ESP_LOGI(TAG, "ui.state %d, ui.index %d, ui.layer %d, ui.select[ui.layer] %d,view length[ui.index] %d", this->ui_v.state, this->ui_v.index, this->ui_v.layer, this->ui_v.select[this->ui_v.layer], ui_lenght);

  TickType_t xLastWakeTime = xTaskGetTickCount(); // 最后唤醒时间
  const TickType_t xFrequency = pdMS_TO_TICKS(10);

  switch (this->ui.state)
  {
  case STATE_LAYER_IN:
    layer_in();
    break;
  case STATE_LAYER_OUT:
    layer_out();
    this->ui.objPage[this->ui.index]->leave();
    break;
  case STATE_FADE:
    this->fade();
    break;
  case STATE_VIEW:
    auto page = this->ui.objPage[this->ui.index];
    this->u8g2->clearBuffer();
    if (!this->ui.init_flag)
    {
      this->ui.init_flag = true;
      page->before();
    }
    page->render();
  }
  xTaskDelayUntil(&xLastWakeTime, xFrequency);
}

const TickType_t xFreequency = 10; // 渲染一帧间隔 /ms
TickType_t xLastWakeTime = xTaskGetTickCount();

void WouoUI::begin()
{
  this->oled_init();

  // 设置屏幕刷新任务
  auto taskUpdate = [](void *pt)
  {
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
    this->ui.objPage[this->ui.index]->onUserInput(this->btnID);
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
  auto ui = &gui->ui;
  auto select = &ui->select[ui->layer];
  auto boxyTarget = &list.box_y_trg[ui->layer];
  int8_t ui_lenght = ((ListPage *)ui->objPage[ui->index])->length;

  switch (btnID)
  {
  case BTN_ID_UP:
    list.box_w_trg += CONFIG_UI[BOX_X_OS]; // 伸展光标
    if (*select == 0)
      break;
    !*boxyTarget
        ? list.text_y_trg += LIST_LINE_H   // 下翻列表
        : *boxyTarget -= LIST_LINE_H;      // 上移光标
    list.box_h_trg += CONFIG_UI[BOX_Y_OS]; // 光标动画
    *select -= 1;                          // 选中行数上移一位
    break;

  case BTN_ID_DO:
    list.box_w_trg += CONFIG_UI[BOX_X_OS]; // 伸展光标
    if (*select == (ui_lenght - 1))
      break;
    *boxyTarget >= (gui->DISPLAY_HEIGHT - LIST_LINE_H)
        ? list.text_y_trg -= LIST_LINE_H   // 上翻列表
        : *boxyTarget += LIST_LINE_H;      // 下移光标
    list.box_h_trg += CONFIG_UI[BOX_Y_OS]; // 光标动画
    *select += 1;                          // 选中行数下移一位
    break;

  case BTN_ID_CANCEL: // 返回
    ESP_LOGI(TAG, "CANCEL");
    gui->ui.select[gui->ui.layer] = 0;
  case BTN_ID_CONFIRM:                     // 确认
    list.box_w_trg += CONFIG_UI[BOX_X_OS]; // 伸展光标
    ESP_LOGI(TAG, "CONFIRM");
    this->router(gui->ui.select[gui->ui.layer]); // 按下确认时执行在派生类中重写的路由方法
    break;
  }

  ui->oper_flag = true;
}

// 列表页面渲染函数
void ListPage::render()
{
  auto *ui_v = &gui->ui;

  // 在每次操作后都会更新的参数
  if (ui_v->oper_flag)
  {
    ui_v->oper_flag = false;

    // 获取选中文本的宽度
    auto box_content_text = view[ui_v->select[ui_v->layer]].m_select;
    list.box_content_width = u8g2->getUTF8Width(box_content_text) + LIST_TEXT_S * 2;

    // 计算滚动条长度
    list.bar_h_trg = ceil(ui_v->select[ui_v->layer] *
                          ((float)gui->DISPLAY_HEIGHT / (this->length - 1)));
  }

  // 计算动画过渡值
  animation(&list.text_x, 0.0f, CONFIG_UI[LIST_ANI]);
  animation(&list.text_y, &list.text_y_trg, CONFIG_UI[LIST_ANI]);
  animation(&list.box_y, &list.box_y_trg[ui_v->layer], CONFIG_UI[LIST_ANI]);
  animation(&list.box_w, &list.box_w_trg, CONFIG_UI[LIST_ANI]);
  animation(&list.box_w_trg, &list.box_content_width, CONFIG_UI[LIST_ANI]);
  animation(&list.box_h, &list.box_h_trg, CONFIG_UI[LIST_ANI]);
  animation(&list.box_h_trg, &list.box_H, CONFIG_UI[LIST_ANI]);
  animation(&list.bar_h, &list.bar_h_trg, CONFIG_UI[LIST_ANI]);

  // 绘制列表文字和行末尾元素
  for (int i = 0; i < this->length; ++i)
  {
    // 绘制文本
    list.text_y_temp = list.text_y + LIST_LINE_H * i;
    list.text_w_temp = list.text_x_temp + LIST_TEXT_W;
    list.text_x_temp = list.text_x * (!CONFIG_UI[COME_SCR]
                                          ? (abs(list.select - i) + 1)
                                          : (i + 1));

    u8g2->setCursor(list.text_x_temp + LIST_TEXT_S, LIST_TEXT_S + LIST_TEXT_H + list.text_y_temp);
    u8g2->print(view[i].m_select);

    //  绘制末尾元素
    switch (view[i].m_select[0])
    {
    case '~':
      list_draw_val(i);
      break;
    case '+':
      this->list_draw_cbf();
      if (check_box.m[i - 1] == 1)
        this->list_draw_cbd();
      break;
    case '=':
      this->list_draw_cbf();
      if (*check_box.s_p == i)
        this->list_draw_cbd();
      break;
    }
  }

  // 绘制进度条和选择框
  u8g2->drawBox(gui->DISPLAY_WIDTH - LIST_BAR_W, 0, LIST_BAR_W, list.bar_h);

  u8g2->drawRBox(0, list.box_y - (list.box_h - LIST_LINE_H) / 2, list.box_w, list.box_h, LIST_BOX_R);
};

void ListPage::before()
{
  auto *ui_v = &gui->ui;

  ui_v->oper_flag = true;
  ESP_LOGI(name, "ui init");
  list.select = ui_v->select[ui_v->layer];
  list.text_x = -gui->DISPLAY_WIDTH;
  list.text_y = list.box_y_trg[ui_v->layer] - LIST_LINE_H * ui_v->select[ui_v->layer];
  list.text_y_trg = list.text_y;
  list.box_H = LIST_LINE_H;

  u8g2->setFont(LIST_FONT);
  u8g2->setDrawColor(2);
}