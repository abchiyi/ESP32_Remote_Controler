#include "setting_power.h"
#include "WouoUI.h"
#include "tool.h"
#include "power.h"

#define TAG "page setting"

LIST_VIEW Setting_power_view;

class L_SETTING_POWER : public ListPage
{
private:
  // 获取 ui 变量地址
  auto get(ui_param_t index)
  {
    return &CONFIG_UI[index];
  }

  uint8_t aa = 100;

public:
  float DC3 = PMU.getDC3Voltage() / 1000.0;
  String result = (String(DC3, 2) + "v");

  void create()
  {
    ListPage::create();
    // com_scr_c.init(&CONFIG_UI[COME_SCR]);
    // radio_1.init(&radio_v, 1);
    // radio_2.init(&radio_v, 2);

    this->view = {
        {"[ Power ]", create_page_jump_fn()},

        {"~ Disp Bri",
         pop_fn<uint8_t>("Disp Bri", &aa, 255, 0, 5),
         create_render_content(aa)},

        {"~ Dc3 sysV",
         pop_fn<float>("DC3 V", &DC3, 255, 0, 5),
         create_render_content(result)}};
  };

  void leave()
  {
    // TODO 执行需要保存配置的对象的 save_config 方法
    // STORAGE_CONFIG.write_all();
  }

  L_SETTING_POWER(LIST_VIEW &_view) : ListPage(_view) {};
};

BasePage *create_page_setting_power()
{
  return new L_SETTING_POWER(Setting_power_view);
}
