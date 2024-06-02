#include <view/setting.h>
#include <view/window.h>
#include <view/about.h>
#include <view/menu.h>
#include <view/setting_devces.h>
#include <storage_config.h>

M_SELECT Setting_view[]{
    {"[ Setting ]"},
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
    {"- [ About ]"},
    {"= Raidio 1"},
    {"= Raidio 2"},
    {"- Devices"},
};

uint8_t aaa = 1;
uint8_t bbb = 13;

class L_SETTING : public ListPage
{

public:
  void create()
  {
    this->setPageView("setting", Setting_view);
    // gui->eepromInit(this);
  }

  void before()
  {
    ListPage::before();
    check_box.v = CONFIG_UI;
    gui->check_box_m_init(CONFIG_UI);
    gui->check_box_v_init(CONFIG_UI);
    gui->check_box_s_init(&aaa, &bbb);

    // gui->eepromReg(this, gui->ui.param);
    // gui->eepromWriteData(this, gui->ui.param);
    // gui->eepromReadData(this, gui->ui.param);
  }

  void router(uint8_t selectItmeNumber)
  {
    UI_VARIABLE *ui = &gui->ui;
    // 设置菜单处理函数
    switch (selectItmeNumber)
    {
    // 返回
    case 0:
      gui->page_out_to(P_MENU);
      break;
    case 1:
      popWindow("Disp Bri", &CONFIG_UI[DISP_BRI], 255, 0, 5, this);
      break;
    case 2:
      popWindow("Box X OS", &CONFIG_UI[BOX_X_OS], 50, 0, 1, this);
      break;
    case 3:
      popWindow("Box Y OS", &CONFIG_UI[BOX_Y_OS], 50, 0, 1, this);
      break;
    case 4:
      popWindow("Win Y OS", &CONFIG_UI[WIN_Y_OS], 40, 0, 1, this);
      break;
    case 5:
      popWindow("List Ani", &CONFIG_UI[LIST_ANI], 255, 20, 1, this);
      break;
    case 6:
      popWindow("Win Ani", &CONFIG_UI[WIN_ANI], 255, 20, 1, this);
      break;
    case 7:
      popWindow("Fade Ani", &CONFIG_UI[FADE_ANI], 255, 0, 1, this);
      break;
    case 8:
      popWindow("Btn SPT", &CONFIG_UI[BTN_SPT], 255, 0, 1, this);
      break;
    case 9:
      popWindow("Btn LPT", &CONFIG_UI[BTN_LPT], 255, 0, 1, this);
      break;
    case 10:
      gui->check_box_m_select(COME_SCR);
      break;
    case 11:
      gui->page_in_to(P_ABOUT);
      break;
    case 12:
      gui->check_box_s_select(0, 12);
      break;
    case 13:
      gui->check_box_s_select(1, 13);
      break;
    case 14:
      gui->page_in_to(P_DEVICES);
      break;
    }
  }

  void leave()
  {
    STORAGE_CONFIG.write_all();
  }
};

ListPage *P_SETTING = new L_SETTING;