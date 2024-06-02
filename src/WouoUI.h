#include <U8g2lib.h>
#include <EEPROM.h>
#include <storage_config.h>

#pragma once

// 按键事件
typedef enum
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
struct
{
  uint8_t select;

  int16_t text_x_temp; // 文本横轴起始点
  int16_t text_y_temp; // 文本纵轴起始点
  int16_t text_w_temp; // 末尾元素横轴起始点

  float text_x;
  float text_x_trg;

  float text_y;
  float text_y_trg;

  float box_y;
  float box_y_trg[UI_DEPTH];

  float box_content_width; // 当前选中文本宽度
  float box_w;
  float box_w_trg;

  float box_H;
  float box_h;
  float box_h_trg;

  float bar_h = 0.0;     // 滚动条目实际长度
  float bar_h_trg = 0.0; // 滚动条目标长度
} list;

// EEPROM变量
#define EEPROM_CHECK 11

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
typedef uint8_t page_index_t;
typedef const char *page_name_t;

void animation(float *a, float *a_trg, uint8_t n);

// 列表页面 view
typedef struct MENU
{
  const char *m_select;
} M_SELECT;

// UI 变量索引
enum
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
};

class BasePage
{
  friend class WouoUI; // 允许 WouoUI 访问受保护的变量
protected:
public:
  U8G2 *u8g2;
  WouoUI *gui; // wouoUI 指针

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
  // 绘制行末尾元素
  // 数值
  void list_draw_val(int n)
  {
    u8g2->setCursor(list.text_w_temp, LIST_TEXT_H + LIST_TEXT_S + list.text_y_temp);
    u8g2->print(check_box.v[n - 1]);
  };

  // 外框
  void list_draw_cbf()
  {
    u8g2->drawRFrame(list.text_w_temp, CB_U + list.text_y_temp, CB_W, CB_H, 0.5f);
  };

  // 点
  void list_draw_cbd()
  {
    u8g2->drawBox(list.text_w_temp + CB_D + 1, CB_U + CB_D + 1 + list.text_y_temp, CB_W - (CB_D + 1) * 2, CB_H - (CB_D + 1) * 2);
  };

  const uint8_t listItemLength = sizeof(M_SELECT);

  template <typename T, size_t N>
  void setPageView(const char *pageName, T (&view)[N])
  {
    this->name = pageName;
    this->length = N;
    this->view = view;
    ESP_LOGI(this->name, "set page lenght - view_length: %d", this->length, this->length);
  };

public:
  M_SELECT *view;           // 列表视图
  uint8_t length = 0;       // 列表视图长度
  void render();            // 渲染函数
  void onUserInput(int8_t); // ListPage 类特定的按键处理函数

  /*
   * @brief 接收当前选中的条目索引作为参数，并执行在派生类中重写后的动作
   */
  virtual void router(uint8_t) = 0;

  virtual void before();
};

typedef struct UI_VARIABLE
{
  bool init_flag;
  bool oper_flag;

  uint16_t buf_len;
  uint8_t *buf_ptr;

  uint8_t fade = 1;

  uint8_t layer;              // 页面嵌套层级
  page_index_t index;         // 当前绘制页面的页码
  uint8_t state = STATE_VIEW; // 页面绘制状态
  uint8_t select[UI_DEPTH];   // list_page 当前选中的条目
  // uint8_t param[UI_PARAM];    // 储存UI参数，如列表动画时间
  BasePage *objPage[UI_DEPTH];
} ui_variable_t;

class WouoUI
{
private:
  U8G2 *u8g2;

  void layer_in();
  void oled_init();

  // 动画
  void fade();
  void layer_out();

public:
  WouoUI(U8G2 *u8g2) : u8g2(u8g2)
  {
    // 获取屏幕宽高
    this->DISPLAY_WIDTH = u8g2->getWidth();
    this->DISPLAY_HEIGHT = u8g2->getHeight();
  };

  uint16_t DISPLAY_HEIGHT; // 屏幕高度 pix
  uint16_t DISPLAY_WIDTH;  // 屏幕宽度 pix

  UI_VARIABLE ui; // 储存ui变量

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

  void pageChange(BasePage *, uint8_t);

  void setDefaultPage(BasePage *);

  void addPage(BasePage *);
  void begin();
  void uiUpdate();
  void btnUpdate(void (*func)(WouoUI *));

  struct size
  {
    uint8_t index;
    uint16_t dataSize;

    size(uint8_t _index, uint16_t _size) : index(_index), dataSize(_size){};
  };
};

extern ConfigHandle<uint8_t[11]> config_ui;
void cb_fn_ui(bool mode);
