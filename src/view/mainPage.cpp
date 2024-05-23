#include <view/mainPage.h>
#include <view/menu.h>
#include <controller.h>
#include <radio.h>
#include <car.h>

class MainPage : public BasePage
{

public:
  void create()
  {
    this->name = "Main page";
    ESP_LOGI(this->name, "page index is %d", this->index);
  };

  void before()
  {
    u8g2->setFont(LIST_FONT);
    u8g2->setDrawColor(2);
    ESP_LOGI(this->name, "set u8g2 font");
  }

  void onUserInput(int8_t btnID)
  {
    switch (btnID)
    {
    case BTN_ID_MENU:
      gui->page_in_to(P_MENU);
      break;
    }
  };

  /**
   * @brief 绘制一个轴向的进度条
   * @param progress -1 ~ 1 的一个浮点数，用于确定进度条百分比
   * @param x x轴起始位置
   * @param y y轴起始位置
   * @param height 高度
   * @param width 宽度
   * @param biaxial 双向摆动模式，根据输入值的正负确定摆动方向
   */
  void drawSlider(float progress, uint8_t x, uint8_t y,
                  uint8_t height = 60, uint8_t width = 5, bool biaxial = false)
  {

    uint8_t padding_bottom = y + height - 1;
    uint8_t padding_left = x + 1;
    uint8_t padding_top = y + 1;
    uint8_t box_width = width - 2;

    uint8_t line_pos_y = y + height / 2; // 中位线y轴坐标
    uint8_t line_end = padding_left + box_width - 1;
    uint8_t line_start = padding_left;

    auto box_y = biaxial
                     ? (progress > 0 ? line_pos_y : line_pos_y + 1)
                     : (y + height - 1);

    /*----- 计算进度 -----*/

    // 计算要填充的最大像素长度
    auto pixel_length = biaxial
                            ? progress > 0
                                  ? line_pos_y - padding_top
                                  : padding_bottom - line_pos_y - 1
                            : height - 2;
    // 实际填充的像素长度
    uint8_t box_height = round(abs(progress * pixel_length));

    auto box_y_pos = (progress < 0 && biaxial) ? box_y : box_y - box_height;

    /*----- 绘制 -----*/

    u8g2->drawFrame(x, y, width, height);                          // 绘制外框
    u8g2->drawBox(padding_left, box_y_pos, box_width, box_height); // 绘制填充
    if (biaxial)                                                   // 绘制中位线
      u8g2->drawLine(line_start, line_pos_y, line_end, line_pos_y);
  }

  /**
   * @brief 绘制一个横向的进度条
   * @param progress -1 ~ 1 的一个浮点数，用于确定进度条百分比
   * @param x x轴起始位置
   * @param y y轴起始位置
   * @param height 高度
   * @param width 宽度
   * @param biaxial 双向摆动模式，根据输入值的正负确定摆动方向
   */
  void drawProgressBar(float progress, uint8_t x, uint8_t y,
                       uint8_t height = 7, float width = 80.0,
                       bool biaxial = false)
  {
    uint8_t padding_right = x + width - 1;
    uint8_t padding_left = x + 1;
    uint8_t padding_top = y + 1;
    uint8_t box_height = height - 2;

    uint8_t line_pos_x = x + width / 2; // 中位线 x 轴坐标
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
                            : width - 2;
    // 实际填充的像素长度
    uint8_t box_width = round(abs(progress * pixel_length));
    auto box_x_pos = (progress < 0 && biaxial) ? box_x - box_width : box_x;

    u8g2->drawBox(box_x_pos, padding_top, box_width, box_height);
    u8g2->drawFrame(x, y, width, height); // 绘制外框
    if (biaxial)                          // 绘制中位线
      u8g2->drawLine(line_pos_x, line_start, line_pos_x, line_end);
  }
  void render()
  {
    u8g2->setCursor(0, 8);
    u8g2->printf("%s | %s",
                 radio.status == RADIO_CONNECTED ? "CONNECTED" : "DISCONNECT", radio.status == RADIO_PAIR_DEVICE ? "P--" : "---");

    float v = Controller.joyLHori / 2048.0;  // 百分比
    float v2 = Controller.joyLVert / 2048.0; // 百分比
    u8g2->setCursor(0, 20);
    u8g2->printf("X %.2f ", v);
    u8g2->setCursor(0, 30);
    u8g2->printf("Y %.2f", v2);

    uint8_t width = 60; // 进度条宽度
    uint8_t height = 4; // 进度条高度
    uint8_t x = 0;      // 进度条x坐标
    uint8_t y = 4;

    this->drawProgressBar(v, 2, 64 - (height * 4 + 4), height, 61);
    this->drawProgressBar(v, 2, 64 - (height * 3 + 3), height, 61, true);

    this->drawProgressBar(v, 2, 64 - (height * 2 + 2), height, width);
    this->drawProgressBar(v, 2, 64 - height, height, width, true);

    this->drawSlider(v2, 70, 15, 40, 4, true);
    this->drawSlider(v2, 80, 15, 41, 4, true);
    this->drawSlider(v2, 90, 15, 40, 4);
    this->drawSlider(v2, 100, 15, 41, 4);

    // auto data = get_channel_status().channel[0];
    // auto gear = (data >> 2) & 0x03;
    // auto brake = data & 0x03;
    // auto value = (data >> 4);

    // auto gear_char = gear == 0   ? "R"
    //                  : gear == 1 ? "D"
    //                              : "N";
    // u8g2->printf("%S BRAKE-%s | %d", gear_char, brake ? "ON " : "OFF", value);
  };
};

BasePage *P_MAIN = new MainPage;