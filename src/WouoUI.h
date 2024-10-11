#include <U8g2lib.h>
#include <EEPROM.h>
#include <vector>
#include <functional>
#include "UI_element.h"

#pragma once

class WouoUI;
extern WouoUI WOUO_UI;

// TODO 重新设计交互触发事件标志
typedef enum key_id
{
  KEY_UP = 1,  // 上翻
  KEY_DOWN,    // 下翻
  KEY_CONFIRM, // 确认
  KEY_BACK,    // 返回
  KEY_MENU,    // 菜单
  KEY_WAKE     // 唤醒
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

typedef struct event_listener
{
  void_cb_fn_t cb_fn;
  key_id_t key;
  event_listener(key_id_t _key_id, void_cb_fn_t _cb_fn)
      : key(_key_id), cb_fn(_cb_fn) {};
} event_listener_t;

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
// #define LIST_FONT u8g2_font_profont12_mf
#define LIST_FONT u8g2_font_HelvetiPixel_tr
#define LIST_LINE_H 16              // 行高
#define LIST_TEXT_W 95              // 末尾元素起始位
#define LIST_TEXT_H LIST_LINE_H / 2 // 文本中轴线对齐高度
#define LIST_TEXT_GAP 4             // 边距
#define LIST_BAR_W 3                // 滚动条宽度
#define LIST_BOX_R 0.5f             // 光标圆角

void animation(float *a, float *a_trg, uint8_t n);
void animation(float *a, float a_trg, uint8_t n);

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

  gui_cb_fn_t check_radio()
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
  char title[WIN_TITLE_W] = "Test window";

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

template <typename T>
class WindowValueSet : public BaseWindow
{
private:
public:
  T *value;
  T max;
  T min;
  T step;

  bool render()
  {

    if (this->box_h > (WIN_TITLE_H + 2 * WIN_TITLE_S))
    {
      u8g2->setCursor(this->box_x + WIN_VALUE_S, this->box_y + WIN_TITLE_S + WIN_TITLE_H);
      u8g2->print(*this->value); // 绘制数值

      u8g2->setCursor(this->box_x + WIN_TITLE_S, this->box_y + WIN_TITLE_S + WIN_TITLE_H);

      // 绘制标题
      u8g2->print(this->title);

      // 绘制进度条
      u8g2->drawBox(this->box_x + WIN_TITLE_S, this->box_y + this->box_h - WIN_TITLE_S - WIN_BAR_H - 1, this->bar_x, WIN_BAR_H);
    }

    return BaseWindow::render();
  }

  WindowValueSet(T *value, T max, T min, T step)
      : value(value), max(max), min(min), step(step)
  {
  }
};

// // 创建弹窗回调
// gui_cb_fn_t pop_window_set_value(const char title[], uint8_t *value, uint8_t max, uint8_t min, uint8_t step)
// {
//   return [=](WouoUI *ui)
//   {
//     BaseWindow *window_pt = new WindowValueSet(value, max, min, step);
//     strcpy(window_pt->title, title);
//     ui->page_pop_window([=]()
//                         { return window_pt; });
//   };
// };

class BasePage
{
  friend class WouoUI; // 允许 WouoUI 访问受保护的变量
private:
protected:
  static BOX CURSOR;

  std::vector<event_listener_t> eventListeners;

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

  TimerHandle_t xTimer_GUI_SLEEP; // GUI休眠定时器
  std::vector<History> history;   // 页面路由

  //

  void layer_in();
  void oled_init();

  // 动画
  void fade();
  void layer_out();

  // 按键事件
  QueueHandle_t Q_Event = xQueueCreate(10, sizeof(event_t));

public:
  uint8_t state = STATE_LAYER_IN; // 页面绘制状态
  uint16_t DISPLAY_HEIGHT;        // 屏幕高度 pix
  uint16_t DISPLAY_WIDTH;         // 屏幕宽度 pix

  // 发送event
  void dispatch_event(event_t event)
  {
    xQueueSend(Q_Event, &event, 10);
  };

  // 注册事件监听
  WouoUI *add_event_listener(key_id_t key_id, void_cb_fn_t cb_fn)
  {
    this->get_history()->eventListeners.push_back(event_listener(key_id, cb_fn));
    return this;
  };

  // 获取顶部窗口
  BaseWindow *get_top_window()
  {
    auto windows = this->get_history()->windows;
    return windows[windows.size() - 1];
  };

  void page_pop_window(create_window_fn_t cb_fn) // 弹出窗口
  {
    auto page = get_history();
    auto window = cb_fn();
    window->gui = this;
    window->u8g2 = u8g2;
    page->windows.push_back(window);
    window->before();
  };
  void gui_sleep();                      // GUI进入睡眠
  void gui_awake();                      // 唤醒GUI
  void page_in_to(create_page_fn_t);     // 进入页面
  void page_back();                      // 退出页面，返回到上一页
  void pageSwitch(BasePage *);           // 切换页面
  void setDefaultPage(create_page_fn_t); // 设置主页面
  void uiUpdate();                       // 归刷
  void begin(U8G2 *u8g2);                // 启动GUI

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

// 创建弹窗回调
template <typename T>
gui_cb_fn_t pop_fn(const char title[], T *value, T max, T min, T step)
{
  return [=](WouoUI *ui)
  {
    BaseWindow *window_pt = new WindowValueSet<T>(value, max, min, step);
    strcpy(window_pt->title, title);
    ui->page_pop_window([=]()
                        { return window_pt; });
  };
}