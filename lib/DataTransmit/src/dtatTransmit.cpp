#include "dtatTransmit.h"

#include "radio.h"
#include "config.h"
#include "tool.h"

extern void recv_setpoint_t(CRTPPacket *packet); // 接收的设定点数据

void init_transmit()
{
    // 初始化无线传输
    init_radio();

    auto bt_CB = [](void) // 蓝牙控制器回调
    {
        auto setpoint_data = get_setpoint_data_from_controller();
        recv_setpoint_t((CRTPPacket *)&setpoint_data);
    };

    // 注册接收回调
    if (CONFIG.radio_mode == BT_CONTROLLER)
        Controller.setCallBack(XBOX_ON_INPUT, bt_CB);
    else
        radio_set_port_callback(CRTP_PORT_SETPOINT, recv_setpoint_t);
}
