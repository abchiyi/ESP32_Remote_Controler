#include <U8g2lib.h>
#include <EEPROM.h>
#include <vector>
#include <functional>
#include "UI_element.h"

#pragma once

typedef enum key_id
{
  KEY_UP = 1,  // 上翻
  KEY_DOWN,    // 下翻
  KEY_CONFIRM, // 确认
  KEY_BACK,    // 返回
  KEY_MENU     // 菜单
} key_id_t;

typedef struct Event
{
  key_id_t key_id;
  Event(key_id_t __key_id) : key_id(__key_id) {};
  Event() {};
} event_t;

class WouoUI;

typedef const char *page_name_t;

typedef std::function<void(WouoUI *)> gui_cb_fn_t; // WouoUI * 指针参数回调
typedef std::function<void()> void_cb_fn_t;        // 无参回调函数类型

typedef struct event_handel
{
  void_cb_fn_t cb_fn;
  key_id_t key;
  event_handel(key_id_t _key_id, void_cb_fn_t _cb_fn)
      : key(_key_id), cb_fn(_cb_fn) {};

} event_handel_t;

#define UI_PARAM 11 // ui 变量总数

// UI状态
typedef enum
{
  STATE_VIEW,
  STATE_BEFORE_VIEW,
  STATE_FADE,
  STATE_LAYER_IN,
  STATE_LAYER_OUT,
  STATE_SWITCH_IN,
  STATE_SWITCH_OUT,
  STATE_SWITCH_VIEW
} ui_state_t;

// 列表变量
#define LIST_FONT u8g2_font_HelvetiPixel_tr
#define LIST_LINE_H 16              // 行高
#define LIST_TEXT_W 100             // 末尾元素起始位
#define LIST_TEXT_H LIST_LINE_H / 2 // 文本中轴线对齐高度
#define LIST_TEXT_S 4               // 边距
#define LIST_BAR_W 3                // 滚动条宽度
#define LIST_BOX_R 0.5f             // 光标圆角

void animation(float *a, float *a_trg, uint8_t n);

// list view unit
struct LIST_VIEW_UNIT
{
  std::string m_select; // 使用 std::string 替代 const char*
  gui_cb_fn_t cb_fn = nullptr;
  gui_cb_fn_t render_end = nullptr;
};

// list view 类型
typedef std::vector<LIST_VIEW_UNIT> LIST_VIEW;

typedef struct check_box_handle
{
  uint8_t *target_val; // 目标变量值
  uint8_t value;       // checkbox 包含值
  gui_cb_fn_t check()  // 选中复选框
  {
    return [=](WouoUI *ui)
    {
      *target_val = !*target_val;
    };
  };

  gui_cb_fn_t chekc_radio()
  {
    return [=](WouoUI *ui)
    {
      *target_val = value;
    };
  }
  void init(uint8_t *t, uint8_t v = 1)
  {
    this->target_val = t;
    this->value = v;
  };
} check_box_handle_t;

// UI 变量索引
typedef enum
{
  DISP_BRI,
  BOX_X_OS,
  BOX_Y_OS,
  WIN_Y_OS,
  LIST_ANI,
  WIN_ANI,
  FADE_ANI,
  BTN_SPT,
  BTN_LPT,
  COME_SCR,
} ui_param_t;

// 弹窗变量
#define WIN_FONT u8g2_font_HelvetiPixel_tr
#define WIN_H 27
#define WIN_W 90
#define WIN_TITLE_W 20
#define WIN_TITLE_H 8
#define WIN_TITLE_S 5
#define WIN_VALUE_S 67
#define WIN_BAR_H 3

/**
 * @brief 基础窗口类型，
 */
class BaseWindow
{
  friend class WouoUI;

private:
  uint8_t a = 100;

  char title[WIN_TITLE_W] = "Test window";
  uint8_t *value = &a;
  uint8_t max = 100;
  uint8_t min = 0;
  uint8_t step = 1;

  U8G2 *u8g2 = nullptr;
  WouoUI *gui = nullptr;

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
  // 此函数为内部调用准备，请勿直接调用
  void __base_render()
  {
    // if (this->render())
    // {
    //   // ESP_LOGI("test", "run close_window");
    // };
    // this->close_window();
  }
  // 关闭窗口，并销毁窗口内存
  void close_window();
  page_name_t name = "pop window";
  void create() {}; // 创建页面，在页面初始化时被调用
  void before();
  void leave();
  // 此函数为内部调用准备，请勿直接调用，返回 false将在合适的时机关闭窗口
  bool render();
  void onUserInput(event_t event);
};

class BasePage
{
  friend class WouoUI; // 允许 WouoUI 访问受保护的变量
private:
protected:
  static BOX CURSOR;

  std::vector<event_handel_t> on_event;

  // 绘制 CURSOR 并计算动画过渡参数
  void draw_cursor()
  {
    CURSOR.x = cursor_position_x;
    CURSOR.y = cursor_position_y;

    animation(&CURSOR.width, &CURSOR.min_width, CURSOR.transition);
    animation(&CURSOR.height, &CURSOR.min_height, CURSOR.transition);

    CURSOR.draw(*u8g2);
  };

  // CURSOR 扩展动画, 对 cursor 的目标值做自＋
  void setCursorOS(uint width_os = 0, uint height_os = 0)
  {
    CURSOR.width += width_os;
    CURSOR.height += height_os;
  }

public:
  std::vector<BaseWindow *> windows; // 窗口

  U8G2 *u8g2 = nullptr;
  WouoUI *gui = nullptr; // wouoUI 指针

  /***** 光标 *****/
  uint8_t select = 0;            // 光标状态（选中第几行）
  float cursor_position_y = 0.0; // 光标坐标 y
  float cursor_position_x = 0.0; // 光标坐标 x

  page_name_t name = "Base page"; // 页面名称

  // 创建页面，在页面初始化时被调用
  virtual void create() {};

  // 在页面渲染前被调用
  virtual void before() {};

  // 在离开页面时调用
  virtual void leave() {};

  // 当有输入时被调用
  virtual void onUserInput(event_t) {}

  // 页面绘制函数， 必须被子类覆盖
  virtual void render() = 0;

  void __base_render()
  {
    render();
  }

  /**
   * @brief 绘制一个轴向的进度条
   * @param progress -1 ~ 1 的一个浮点数，用于确定进度条百分比
   * @param x x轴起始位置
   * @param y y轴起始位置
   * @param width 滑动条宽度
   * @param length 滑动条长度
   * @param biaxial 双向摆动模式，根据输入值的正负确定摆动方向
   */
  void draw_slider_y(float progress,
                     uint8_t x, uint8_t y, uint8_t width = 4,
                     uint8_t length = 60, bool biaxial = false);

  /**
   * @brief 绘制一个横向的进度条
   * @param progress -1 ~ 1 的一个浮点数，用于确定进度条百分比
   * @param x x轴起始位置
   * @param y y轴起始位置
   * @param width 滑动条宽度
   * @param length 滑动条长度
   * @param biaxial 双向摆动模式，根据输入值的正负确定摆动方向
   */
  void draw_slider_x(float progress,
                     uint8_t x, uint8_t y, uint8_t width = 4,
                     uint8_t length = 60, bool biaxial = false);
};

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
  int16_t text_w_temp; // 文本起始起始坐标

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

  // private:
  gui_cb_fn_t create_render_checxbox(check_box_handle &cbh)
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
  };

  // 绘制行末尾数值
  template <typename T>
  gui_cb_fn_t create_render_content(T *content)
  {
    return [=](WouoUI *ui)
    {
      u8g2->setCursor(text_w_temp, LIST_TEXT_H + LIST_TEXT_S + text_y_temp);
      u8g2->print(*content);
    };
  };

public:
  LIST_VIEW &view;        // 列表视图
  void render() override; // 渲染函数
  void create();
  ListPage(LIST_VIEW &view) : view(view) {};
  virtual void before();
};

typedef enum
{
  H_VIEW,
  H_WINDOW,
} history_mode_t;

typedef std::function<BasePage *()> create_page_fn_t;
typedef std::function<BaseWindow *()> create_window_fn_t;

struct History
{
  BasePage *page;
  history_mode_t mode;

  History(BasePage *page) : page(page) {};
};

class WouoUI
{
private:
  U8G2 *u8g2;
  uint16_t buf_len;
  uint8_t *buf_ptr;

  std::vector<History> history; // 页面路由

  void layer_in();
  void oled_init();

  // 动画
  void fade();
  void layer_out();

  // 按键事件
  QueueHandle_t Q_Event = xQueueCreate(10, sizeof(event_t));

public:
  // 发送event
  void dispatchEvent(event_t event)
  {
    // TODO 配置事件发送等待时间
    xQueueSend(Q_Event, &event, 10);
  };

  //
  event_t getEvent()
  {
    event_t event;
    xQueueReceive(Q_Event, &event, 1);
    return event;
  };

  WouoUI *on(key_id_t key_id, void_cb_fn_t cb_fn)
  {
    this->get_history()->on_event.push_back(event_handel(key_id, cb_fn));
    return this;
  };

  uint8_t state = STATE_LAYER_IN; // 页面绘制状态
  WouoUI(U8G2 *u8g2) : u8g2(u8g2)
  {
    // 获取屏幕宽高
    this->DISPLAY_WIDTH = u8g2->getWidth();
    this->DISPLAY_HEIGHT = u8g2->getHeight();
  };

  uint16_t DISPLAY_HEIGHT; // 屏幕高度 pix
  uint16_t DISPLAY_WIDTH;  // 屏幕宽度 pix

  void page_pop_window(create_window_fn_t cb_fn)
  {
    auto page = get_history();
    auto window = cb_fn();
    window->gui = this;
    window->u8g2 = u8g2;
    page->windows.push_back(window);
    window->before();
  }

  BaseWindow *windows_top()
  {
    auto windows = this->get_history()->windows;
    return windows[windows.size() - 1];
  };

  void page_in_to(create_page_fn_t);
  void page_back();
  void pageSwitch(BasePage *);

  void setDefaultPage(create_page_fn_t);
  void uiUpdate();
  void begin(U8G2 *u8g2);

  /*
   * @brief 获取历史路由
   * @param index 路由序号
   */
  BasePage *get_history(int index = -1);
};

/**
 * @brief 简化跳转回调函数写法，
 * 对于只需要执行跳转动作的选项可以使用此函数生成跳转函数
 * @param mode 决定页码跳转模式
 * @param cb_fn 页面创建函数
 */
gui_cb_fn_t create_page_jump_fn(create_page_fn_t cb_fn = nullptr);

extern uint8_t CONFIG_UI[UI_PARAM];