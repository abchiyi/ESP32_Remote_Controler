#include <U8g2lib.h>
#include <EEPROM.h>
#include <storage_config.h>
#include <vector>
#include <functional>

#pragma once

typedef enum key_event
{
  BTN_ID_UP,      // 上翻
  BTN_ID_DO,      // 下翻
  BTN_ID_CONFIRM, // 确认
  BTN_ID_CANCEL,  // 返回
  BTN_ID_MENU     // 菜单
} key_event_t;

// UI变量
#define UI_DEPTH 256 // 页面栈深度
#define UI_PARAM 11  // ui 变量总数

// UI状态
#define STATE_VIEW 0x00
#define STATE_FADE 0x01
#define STATE_LAYER_IN 0x02
#define STATE_LAYER_OUT 0x03

// 列表变量
#define LIST_FONT u8g2_font_HelvetiPixel_tr
#define LIST_LINE_H 16              // 行高
#define LIST_TEXT_W 100             // 末尾元素起始位
#define LIST_TEXT_H LIST_LINE_H / 2 // 文本中轴线对齐高度
#define LIST_TEXT_S 4               // 边距
#define LIST_BAR_W 3                // 滚动条宽度
#define LIST_BOX_R 0.5f             // 光标圆角

// 选择框变量
#define CB_U 2
#define CB_W 12
#define CB_H 12
#define CB_D 2
struct
{
  uint8_t *v;
  uint8_t *m;
  uint8_t *s;
  uint8_t *s_p;
} check_box;
class WouoUI;
typedef const char *page_name_t;

void animation(float *a, float *a_trg, uint8_t n);

typedef std::function<void(WouoUI *)> view_cb_t;

// list view unit
struct LIST_VIEW_UNIT
{
  const char *m_select;
  view_cb_t cb_fn = nullptr;
};

// list view 类型
typedef std::vector<LIST_VIEW_UNIT> LIST_VIEW;

// UI 变量索引
typedef  enum ui_param
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

typedef enum Page_Jump_Mode
{
  PAGE_IN,
  PAGE_OUT,
} page_jump_mode_t;

class BasePage
{
  friend class WouoUI; // 允许 WouoUI 访问受保护的变量
protected:
public:
  U8G2 *u8g2;
  WouoUI *gui; // wouoUI 指针

  /***** 光标 *****/

  uint8_t select = 0; // 光标状态（选中第几行）

  float box_y_trg = 0.0; // 光标纵轴目标坐标(为每个页面储存一个独立的光标位置)
  static float box_w_trg;
  static float box_h_trg;

  page_name_t name = "Base page"; // 页面名称
  uint8_t index;                  // 页码

  // 创建页面，在页面初始化时被调用
  virtual void create() {};

  // 在页面渲染前被调用
  virtual void before() {};

  // 在离开页面时调用
  virtual void leave() {};

  // 当有输入时被调用
  virtual void onUserInput(int8_t) {}

  // 页面绘制函数， 必须被子类覆盖
  virtual void render() = 0;

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

  template <typename T, size_t N>
  void setPageView(const char *pageName, T (&view)[N]) {
  };

public:
  LIST_VIEW &view;          // 列表视图
  void render();            // 渲染函数
  void onUserInput(int8_t); // ListPage 类特定的按键处理函数

  ListPage(LIST_VIEW &view) : view(view){};

  virtual void before();
};

class WouoUI
{
private:
  U8G2 *u8g2;

  uint16_t buf_len;
  uint8_t *buf_ptr;

  void layer_in();
  void oled_init();

  // 动画
  void fade();
  void layer_out();

public:
  uint8_t state = STATE_VIEW; // 页面绘制状态
  WouoUI(U8G2 *u8g2) : u8g2(u8g2)
  {
    // 获取屏幕宽高
    this->DISPLAY_WIDTH = u8g2->getWidth();
    this->DISPLAY_HEIGHT = u8g2->getHeight();
  };

  uint16_t DISPLAY_HEIGHT; // 屏幕高度 pix
  uint16_t DISPLAY_WIDTH;  // 屏幕宽度 pix

  BasePage *objPage[UI_DEPTH]; // 所有已注册的页面
  uint8_t layer;               // 页面嵌套层级
  uint8_t index;               // 当前页面的页码
  uint8_t index_targe;         // 目标页面的页码

  bool init_flag;
  bool oper_flag;

  void check_box_v_init(uint8_t *);
  void check_box_m_init(uint8_t *);
  void check_box_s_init(uint8_t *param, uint8_t *param_p);
  void check_box_s_select(uint8_t val, uint8_t pos);
  void check_box_m_select(uint8_t param);

  volatile bool btnPressed = false; // 按键事件处理标志
  volatile int8_t btnID;            // 按键事件触发时在这里读取按钮ID

  void page_in_to(BasePage *);
  void page_out_to(BasePage *);
  void pageSwitch(BasePage *);

  void setDefaultPage(BasePage *);

  void addPage(BasePage *);
  void begin(U8G2 *u8g2);
  void uiUpdate();
  void btnUpdate(void (*func)(WouoUI *));

  /*
   * @brief 根据页码获取页面对象
   * @param index 页码
   */
  auto getPage(uint8_t index)
  {
    return this->objPage[index];
  }

  /*
   * @brief 根据页码获取页面对象
   */
  auto getPage()
  {
    return this->objPage[this->index];
  }
};

extern ConfigHandle<uint8_t[11]> config_ui;
void cb_fn_ui(bool mode);

/**
 * @brief 简化跳转回调函数写法，
 * 对于只需要执行跳转动作的选项可以使用此函数生成跳转函数
 * @param mode 决定页码跳转模式
 * @param page 目标页面
 */
view_cb_t create_page_jump_fn(page_jump_mode_t mode, BasePage *&page);
