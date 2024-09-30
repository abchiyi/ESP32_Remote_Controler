#include <view/setting.h>
#include <view/about.h>
#include "tool.h"
#include "radio.h"
#include "setting_power.h"

#define TAG "page setting"

LIST_VIEW Setting_view;

class L_SETTING : public ListPage
{
private:
  check_box_handle_t com_scr_c;
  uint8_t radio_v = 1;
  check_box_handle_t radio_1;
  check_box_handle_t radio_2;

  // 获取 ui 变量地址
  auto get(ui_param_t index)
  {
    return &CONFIG_UI[index];
  }

public:
  void create()
  {
    ListPage::create();
    com_scr_c.init(&CONFIG_UI[COME_SCR]);
    radio_1.init(&radio_v, 1);
    radio_2.init(&radio_v, 2);

    uint8_t aa = 100;

    this->view = {
        {"[ Setting ]", create_page_jump_fn()},
        {"- [Power]", create_page_jump_fn(create_page_setting_power)},

        {"~ Disp Bri",
         pop_fn<uint8_t>("Disp Bri", get(DISP_BRI), 255, 0, 5),
         create_render_content(*get(DISP_BRI))},

        {"~ Box X OS",
         pop_fn<uint8_t>("Box X OS", get(BOX_X_OS), 50, 0, 1),
         create_render_content(*get(BOX_X_OS))},

        {"~ Box Y OS",
         pop_fn<uint8_t>("Box Y OS", get(BOX_Y_OS), 50, 0, 1),
         create_render_content(*get(BOX_Y_OS))},

        {"~ Win Y OS",
         pop_fn<uint8_t>("Win Y OS", get(WIN_Y_OS), 40, 0, 1),
         create_render_content(*get(WIN_Y_OS))},

        {"~ List Ani",
         pop_fn<uint8_t>("List Ani", get(LIST_ANI), 255, 20, 1),
         create_render_content(*get(LIST_ANI))},

        {"~ Win Ani",
         pop_fn<uint8_t>("Win Ani", get(WIN_ANI), 255, 20, 1),
         create_render_content(*get(WIN_ANI))},

        {"~ Fade Ani",
         pop_fn<uint8_t>("Fade Ani", get(FADE_ANI), 255, 0, 1),
         create_render_content(*get(FADE_ANI))},

        {"~ Btn SPT",
         pop_fn<uint8_t>("Btn SPT", get(BTN_SPT), 255, 0, 1),
         create_render_content(*get(BTN_SPT))},

        {"~ Btn LPT",
         pop_fn<uint8_t>("Btn LPT", get(BTN_LPT), 255, 0, 1),
         create_render_content(*get(BTN_LPT))},

        {"+ Come Fm Scr",
         com_scr_c.check(),
         create_render_checkbox(com_scr_c)},

        {"= Radio 1",
         radio_1.check_radio(),
         create_render_checkbox(radio_1)},

        {"= Radio 2",
         radio_2.check_radio(),
         create_render_checkbox(radio_2)},

        {"- Rest", [&](WouoUI *ui)
         {
           RADIO.config_clear();
         }},

        {"- [ About ]", [&](WouoUI *ui)
         {
           ui->page_in_to(create_page_about);
         }},
    };
  };

  void leave()
  {
    // TODO 执行需要保存配置的对象的 save_config 方法
    // STORAGE_CONFIG.write_all();
  }

  L_SETTING(LIST_VIEW &_view) : ListPage(_view) {};
};

BasePage *create_page_setting()
{
  return new L_SETTING(Setting_view);
}
