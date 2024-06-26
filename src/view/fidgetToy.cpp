#include <view/fidgetToy.h>
#include <view/menu.h>
#include <view/setting.h>

// 解压玩具变量
#define F0_PARAM 3
#define F0_BOX_H 20
#define F0_BOX_W 20
#define F0_POS_N 4
#define F0_BOX_X 0
#define F0_BOX_Y 1
#define F0_BOX_R 2
enum
{
  F0_X_OS,
  F0_Y_OS,
  F0_ANI
};
struct
{
  int8_t select;
  uint8_t param[F0_PARAM];
} f0;

class F0toy : public BasePage
{

public:
  uint8_t pos[F0_POS_N][2];

  void create() {

  };

  void before()
  {
    pos[0][0] = 0;
    pos[0][1] = 0;

    pos[1][0] = gui->DISPLAY_WIDTH - F0_BOX_W;
    pos[1][1] = 0;

    pos[2][0] = gui->DISPLAY_WIDTH - F0_BOX_W;
    pos[2][1] = gui->DISPLAY_HEIGHT - F0_BOX_H;

    pos[3][0] = 0;
    pos[3][1] = gui->DISPLAY_HEIGHT - F0_BOX_H;

    CURSOR.transition = CONFIG_UI[LIST_ANI];
    CURSOR.min_width = F0_BOX_W;
    CURSOR.min_height = F0_BOX_H;
    CURSOR.round_corner = 1;

    // 功能初始化
    f0.param[F0_X_OS] = 40;
    f0.param[F0_Y_OS] = 40;
    f0.param[F0_ANI] = 100;

    // 进场时元素移动到初始位置

    setCursorOS(f0.param[F0_X_OS], f0.param[F0_Y_OS]);
    cursor_position_x = 0;
    cursor_position_y = 0;

    // 其它初始化
    u8g2->setDrawColor(1);
  }

  void onUserInput(int8_t btnID)
  {
    ESP_LOGI(this->name, "F0toy");
    switch (btnID)
    {
    case KEY_UP:
      f0.select++;
      if (f0.select > F0_POS_N - 1)
        f0.select = 0;
      break;
    case KEY_DOWN:
      f0.select--;
      if (f0.select < 0)
        f0.select = F0_POS_N - 1;
      break;
    case KEY_CONFIRM:
    case KEY_BACK:
      this->gui->page_back();
      break;
    }

    cursor_position_x = pos[f0.select][F0_BOX_X];
    cursor_position_y = pos[f0.select][F0_BOX_Y];
  }

  void render()
  {
    // 在每次操作后都会更新的参数

    // if (CURSOR.x != box_x_trg)
    //   box_w_trg += f0.param[F0_X_OS];
    // if (CURSOR.y != box_y_trg)
    //   box_h_trg += f0.param[F0_Y_OS];

    draw_cursor();
  };
};

BasePage *create_page_f0_toy()
{
  return new F0toy;
}
