#include <view/setting.h>
#include <view/about.h>
#include "tool.h"
#include "radio.h"

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

  // 创建弹窗回调
  gui_cb_fn_t pop_fn(const char title[], uint8_t *value, uint8_t max, uint8_t min, uint8_t step)
  {
    return [=](WouoUI *ui)
    {
      auto window_pt = new BaseWindow;
      strcpy(window_pt->title, title);
      window_pt->value = value;
      window_pt->max = max;
      window_pt->min = min;
      window_pt->step = step;
      ui->page_pop_window([=]()
                          { return window_pt; });
    };
  };

public:
  void create()
  {
    ListPage::create();
    com_scr_c.init(&CONFIG_UI[COME_SCR]);
    radio_1.init(&radio_v, 1);
    radio_2.init(&radio_v, 2);

    this->view = {
        {"[ Setting ]", create_page_jump_fn()},

        {"~ Disp Bri",
         pop_fn("Disp Bri", get(DISP_BRI), 255, 0, 5),
         create_render_content(get(DISP_BRI))},

        {"~ Box X OS",
         pop_fn("Box X OS", get(BOX_X_OS), 50, 0, 1),
         create_render_content(get(BOX_X_OS))},

        {"~ Box Y OS",
         pop_fn("Box Y OS", get(BOX_Y_OS), 50, 0, 1),
         create_render_content(get(BOX_Y_OS))},

        {"~ Win Y OS",
         pop_fn("Win Y OS", get(WIN_Y_OS), 40, 0, 1),
         create_render_content(get(WIN_Y_OS))},

        {"~ List Ani",
         pop_fn("List Ani", get(LIST_ANI), 255, 20, 1),
         create_render_content(get(LIST_ANI))},

        {"~ Win Ani",
         pop_fn("Win Ani", get(WIN_ANI), 255, 20, 1),
         create_render_content(get(WIN_ANI))},

        {"~ Fade Ani",
         pop_fn("Fade Ani", get(FADE_ANI), 255, 0, 1),
         create_render_content(get(FADE_ANI))},

        {"~ Btn SPT",
         pop_fn("Btn SPT", get(BTN_SPT), 255, 0, 1),
         create_render_content(get(BTN_SPT))},

        {"~ Btn LPT",
         pop_fn("Btn LPT", get(BTN_LPT), 255, 0, 1),
         create_render_content(get(BTN_LPT))},

        {"+ Come Fm Scr",
         com_scr_c.check(),
         create_render_checxbox(com_scr_c)},

        {"= Raidio 1",
         radio_1.chekc_radio(),
         create_render_checxbox(radio_1)},

        {"= Raidio 2",
         radio_2.chekc_radio(),
         create_render_checxbox(radio_2)},

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
