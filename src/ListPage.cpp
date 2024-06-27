#include "ListPage.h"
#define TAG "List Page type"

float ListPage::text_y;
float ListPage::text_x;
float ListPage::text_x_trg;
float ListPage::text_y_trg;

void ListPage::create()
{
  gui->add_event_listener(KEY_UP, [&]()
                          { cursorMoveUP(); })
      ->add_event_listener(KEY_DOWN, [&]()
                           { cursorMoveDOWN(); })
      ->add_event_listener(KEY_BACK, [&]()
                           { 
            auto fn = view[0].cb_fn; 
           if(fn)fn(gui); })
      ->add_event_listener(KEY_CONFIRM, [&]
                           {
   if (this->view[select].cb_fn)
            this->view[select].cb_fn(gui);
          setCursorOS(CONFIG_UI[BOX_X_OS]); });
};

void ListPage::render()
{
  auto length = this->view.size();
  auto page = gui->get_history();

  static int16_t text_x_temp; // 文本横轴起始坐标

  static uint8_t list_ani = CONFIG_UI[LIST_ANI];
  static uint8_t com_scr = CONFIG_UI[COME_SCR];

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

    // 绘制列表文字和行末尾元素
    for (int i = 0; i < length; ++i)
    {
      // 绘制文本
      text_y_temp = text_y + LIST_LINE_H * i;
      text_x_temp = text_x * (!com_scr ? (abs(gui->get_history()->select - i) + 1)
                                       : (i + 1));
      text_w_temp = text_x_temp + LIST_TEXT_W;

      u8g2->setCursor(text_x_temp + LIST_TEXT_S, LIST_TEXT_S + LIST_TEXT_H + text_y_temp);
      u8g2->print(view[i].m_select.c_str());
      if (view[i].render_end)
        view[i].render_end(gui);
    }
  };

  auto render_bar = [&]()
  {
    this->bar_h_trg = ceil(gui->get_history()->select *
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
  ESP_LOGI(TAG, "list page before");
  text_x = -gui->DISPLAY_WIDTH;
  text_y = cursor_position_y - LIST_LINE_H * select;
  text_y_trg = text_y;

  // CURSOR
  CURSOR.x = 0;
  CURSOR.round_corner = 0.5;
  CURSOR.min_height = LIST_LINE_H;
  CURSOR.transition = CONFIG_UI[LIST_ANI];

  u8g2->setFont(LIST_FONT);
  u8g2->setDrawColor(2);
};

void ListPage::cursorMoveDOWN(uint step)
{
  uint8_t box_x_os = CONFIG_UI[BOX_X_OS];

  auto move = [&]()
  {
    if (select != (((ListPage *)gui->get_history())->view.size() - 1))
    {
      setCursorOS(box_x_os, CONFIG_UI[BOX_Y_OS]); // 光标轮廓扩大
      select += 1;                                // 选中行数下移一位
      // 光标到达屏幕底部
      cursor_position_y < (gui->DISPLAY_HEIGHT - LIST_LINE_H)
          ? cursor_position_y += LIST_LINE_H // 下移光标
          : text_y_trg -= LIST_LINE_H;       // 上翻列表
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
  uint8_t box_x_os = CONFIG_UI[BOX_X_OS];

  auto move = [&]()
  {
    if (select != 0)
    {
      setCursorOS(box_x_os, CONFIG_UI[BOX_Y_OS]); // 光标轮廓扩大
      select -= 1;                                // 选中行数上移一位
      cursor_position_y                           // 当光标未到达屏幕顶部时
          ? cursor_position_y -= LIST_LINE_H      // 上移光标
          : text_y_trg += LIST_LINE_H;            // 下翻列表
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

gui_cb_fn_t ListPage::create_render_checxbox(check_box_handle_t &cbh)
{
  // 选择框变量
  static const uint8_t CB_U = 2;
  static const uint8_t CB_W = 12;
  static const uint8_t CB_H = 12;
  static const uint8_t CB_D = 2;

  return [=](WouoUI *ui)
  {
    // 外框
    u8g2->drawRFrame(text_w_temp, CB_U + text_y_temp, CB_W, CB_H, 0.5f);
    if (*cbh.target_val == cbh.value)
      // 点
      u8g2->drawBox(text_w_temp + CB_D + 1, CB_U + CB_D + 1 + text_y_temp, CB_W - (CB_D + 1) * 2, CB_H - (CB_D + 1) * 2);
  };
}
