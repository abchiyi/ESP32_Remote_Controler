#include <view/setting.h>
#include <view/window.h>
#include <view/about.h>
#include <view/menu.h>
#include <view/setting_devces.h>
#include <storage_config.h>

#define TAG "page setting"

LIST_VIEW Setting_view;

uint8_t aaa = 1;
uint8_t bbb = 13;

class L_SETTING : public ListPage
{
private:
  // 获取 ui 变量地址
  auto get(ui_param_t index)
  {
    return &config_ui.ref[index];
  }

  // 创建弹窗回调
  view_cb_t pop_fn(const char title[], uint8_t *value, uint8_t max, uint8_t min, uint8_t step)
  {
    return [=](WouoUI *ui)
    {
      popWindow(title, value, max, min, step, P_SETTING);
    };
  };

public:
  void
  create()
  {
    Setting_view = {
        {"[ Setting ]", create_page_jump_fn(PAGE_OUT, P_MENU)},
        {"~ Disp Bri", pop_fn("Disp Bri", get(DISP_BRI), 255, 0, 5)},
        {"~ Box X OS", pop_fn("Box X OS", get(BOX_X_OS), 50, 0, 1)},
        {"~ Box Y OS", pop_fn("Box Y OS", get(BOX_Y_OS), 50, 0, 1)},
        {"~ Win Y OS", pop_fn("Win Y OS", get(WIN_Y_OS), 40, 0, 1)},
        {"~ List Ani", pop_fn("List Ani", get(LIST_ANI), 255, 20, 1)},
        {"~ Win Ani", pop_fn("Win Ani", get(WIN_ANI), 255, 20, 1)},
        {"~ Fade Ani", pop_fn("Fade Ani", get(FADE_ANI), 255, 0, 1)},
        {"~ Btn SPT", pop_fn("Btn SPT", get(BTN_SPT), 255, 0, 1)},
        {"~ Btn LPT", pop_fn("Btn LPT", get(BTN_LPT), 255, 0, 1)},
        {"+ Come Fm Scr", [&](WouoUI *ui)
         {
           ui->check_box_m_select(COME_SCR);
         }},
        {"- [ About ]", create_page_jump_fn(PAGE_IN, P_ABOUT)},
        {"= Raidio 1", [&](WouoUI *ui)
         {
           ui->check_box_s_select(0, 12);
         }},
        {"= Raidio 2", [&](WouoUI *ui)
         {
           ui->check_box_s_select(1, 13);
         }},
        {"- Rest", [&](WouoUI *ui)
         { STORAGE_CONFIG.clearEEPROM(); }},
    };
  };

  void before()
  {
    ListPage::before();
    check_box.v = config_ui.ref;
    gui->check_box_m_init(config_ui.ref);
    gui->check_box_v_init(config_ui.ref);
    gui->check_box_s_init(&aaa, &bbb);
  }

  void leave()
  {
    STORAGE_CONFIG.write_all();
  }

  L_SETTING(LIST_VIEW &_view) : ListPage(_view){};
};

BasePage *P_SETTING = new L_SETTING(Setting_view);