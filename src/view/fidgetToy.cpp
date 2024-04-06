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
struct DD
{
  float box_x;
  float box_x_trg;

  float box_y;
  float box_y_trg;

  float box_W = F0_BOX_W;
  float box_w;
  float box_w_trg;

  float box_H = F0_BOX_H;
  float box_h;
  float box_h_trg;

  int8_t select;

  uint8_t param[F0_PARAM];
} f0;

class F0toy : public BasePage
{

public:
  uint8_t pos[F0_POS_N][2];
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

    auto *ui = &this->gui->ui;

    // 功能初始化
    f0.param[F0_X_OS] = 40;
    f0.param[F0_Y_OS] = 40;
    f0.param[F0_ANI] = 100;

    // 进场时元素从屏幕外入场
    f0.box_x = -F0_BOX_W;
    f0.box_y = -F0_BOX_H;
    ui->oper_flag = true;

    // 进场时元素移动到初始位置
    f0.box_x_trg = 0;
    f0.box_y_trg = 0;

    // 进场时元素效果
    f0.box_w_trg += f0.param[F0_X_OS];
    f0.box_h_trg += f0.param[F0_Y_OS];

    // 其它初始化
    u8g2->setDrawColor(1);
  }

  void onUserInput(int8_t btnID)
  {
    ESP_LOGI(this->name, "F0toy");
    gui->ui.oper_flag = true;
    switch (btnID)
    {
    case BTN_ID_UP:
      f0.select++;
      if (f0.select > F0_POS_N - 1)
        f0.select = 0;
      break;
    case BTN_ID_DO:
      f0.select--;
      if (f0.select < 0)
        f0.select = F0_POS_N - 1;
      break;
    case BTN_ID_CONFIRM:
    case BTN_ID_CANCEL:
      this->gui->page_out_to(P_MENU);
      break;
    }
    f0.box_x_trg = pos[f0.select][F0_BOX_X];
    f0.box_y_trg = pos[f0.select][F0_BOX_Y];
  }

  void render()
  {
    auto *ui = &this->gui->ui;

    // 在每次操作后都会更新的参数
    if (ui->oper_flag)
    {
      ui->oper_flag = false;
      if (f0.box_x != f0.box_x_trg)
        f0.box_w_trg += f0.param[F0_X_OS];
      if (f0.box_y != f0.box_y_trg)
        f0.box_h_trg += f0.param[F0_Y_OS];
    }

    // 计算动画过渡值
    animation(&f0.box_x, &f0.box_x_trg, f0.param[F0_ANI]);
    animation(&f0.box_y, &f0.box_y_trg, f0.param[F0_ANI]);
    animation(&f0.box_w, &f0.box_w_trg, f0.param[F0_ANI]);
    animation(&f0.box_w_trg, &f0.box_W, f0.param[F0_ANI]);
    animation(&f0.box_h, &f0.box_h_trg, f0.param[F0_ANI]);
    animation(&f0.box_h_trg, &f0.box_H, f0.param[F0_ANI]);

    // 绘制元素
    u8g2->drawRBox((int16_t)f0.box_x, (int16_t)f0.box_y, f0.box_w, f0.box_h, F0_BOX_R);
  };
};

BasePage *F0TOY = new F0toy;
