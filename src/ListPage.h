#include "WouoUI.h"
#pragma once

class ListPage : public BasePage
{
protected:
  float bar_h = 0.0;     // 滚动条目实际长度
  float bar_h_trg = 0.0; // 滚动条目标长度

  static float text_x;
  static float text_x_trg;
  static float text_y;
  static float text_y_trg;

  int16_t text_y_temp; // 文本纵轴起始坐标
  int16_t text_x_temp; // 文本横轴起始坐标

  /**
   * @brief 向上移动光标，当光标移动到屏幕顶部，且列表未到达顶部则向下卷动列表
   * @param step 执行一次移动光标多少行
   */
  void cursorMoveUP(uint step = 1);

  /**
   * @brief 向下移动光标，当光标移动到屏幕底部，且列表未到达顶底则向上卷动列表
   * @param step 执行一次移动光标多少行
   */
  void cursorMoveDOWN(uint step = 1);

  // 创建选择框绘制函数
  gui_cb_fn_t create_render_checkbox(check_box_handle_t &);

  // 创建内容绘制回调函数
  template <typename T> // 模板函数需要在头文件中定义
  gui_cb_fn_t create_render_content(T content)
  {
    return [=](WouoUI *ui)
    {
      auto text_width = u8g2->getStrWidth(String(content).c_str());
      auto text_w_temp =       // 元素起始点
          gui->DISPLAY_WIDTH   // 屏幕宽度
          + text_x_temp        // 文本横轴起始点，负数~0的值
          - text_width         // 内容宽度
          - LIST_TEXT_GAP * 2; // 末尾留白

      u8g2->setCursor(text_w_temp,
                      LIST_TEXT_H + LIST_TEXT_GAP + text_y_temp);
      u8g2->print(content);
    };
  };

public:
  LIST_VIEW &view; // 列表视图
  void render();
  void create();
  ListPage(LIST_VIEW &view) : view(view) {};
  virtual void before();
};
