#include <view/setting.h>
#include <view/window.h>
#include <view/about.h>
#include <view/menu.h>
#include <view/setting_devces.h>
#include <storage_config.h>

#define TAG "page setting"

LIST_VIEW Setting_view{
    {"[ Setting ]", create_page_jump_fn(PAGE_OUT, P_MENU)},
    {"~ Disp Bri"},
    {"~ Box X OS"},
    {"~ Box Y OS"},
    {"~ Win Y OS"},
    {"~ List Ani"},
    {"~ Win Ani"},
    {"~ Fade Ani"},
    {"~ Btn SPT"},
    {"~ Btn LPT"},
    {"+ Come Fm Scr"},
    {"- [ About ]", create_page_jump_fn(PAGE_IN, P_ABOUT)},
    {"= Raidio 1"},
    {"= Raidio 2"},
    {"- Rest", [&](WouoUI *ui)
     { STORAGE_CONFIG.clearEEPROM(); }},
};

uint8_t aaa = 1;
uint8_t bbb = 13;

class L_SETTING : public ListPage
{

public:
  void before()
  {
    ListPage::before();
    // check_box.v = config_ui.ref;
    // gui->check_box_m_init(config_ui.ref);
    // gui->check_box_v_init(config_ui.ref);
    // gui->check_box_s_init(&aaa, &bbb);
  }

  // switch (selectItmeNumber)
  // {
  // case 1:
  //   popWindow("Disp Bri", &config_ui.ref[DISP_BRI], 255, 0, 5, this);
  //   break;
  // case 2:
  //   popWindow("Box X OS", &config_ui.ref[BOX_X_OS], 50, 0, 1, this);
  //   break;
  // case 3:
  //   popWindow("Box Y OS", &config_ui.ref[BOX_Y_OS], 50, 0, 1, this);
  //   break;
  // case 4:
  //   popWindow("Win Y OS", &config_ui.ref[WIN_Y_OS], 40, 0, 1, this);
  //   break;
  // case 5:
  //   popWindow("List Ani", &config_ui.ref[LIST_ANI], 255, 20, 1, this);
  //   break;
  // case 6:
  //   popWindow("Win Ani", &config_ui.ref[WIN_ANI], 255, 20, 1, this);
  //   break;
  // case 7:
  //   popWindow("Fade Ani", &config_ui.ref[FADE_ANI], 255, 0, 1, this);
  //   break;
  // case 8:
  //   popWindow("Btn SPT", &config_ui.ref[BTN_SPT], 255, 0, 1, this);
  //   break;
  // case 9:
  //   popWindow("Btn LPT", &config_ui.ref[BTN_LPT], 255, 0, 1, this);
  //   break;
  // case 10:
  //   gui->check_box_m_select(COME_SCR);
  //   break;
  // case 12:
  //   gui->check_box_s_select(0, 12);
  //   break;
  // case 13:
  //   gui->check_box_s_select(1, 13);
  //   break;
  // }

  void leave()
  {
    STORAGE_CONFIG.write_all();
  }

  L_SETTING(LIST_VIEW &_view) : ListPage(_view){};
};

BasePage *P_SETTING = new L_SETTING(Setting_view);