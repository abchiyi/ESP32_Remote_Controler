#include "dataTransmit.h"

#include "radio.h"
#include "config.h"
#include "tool.h"

#define TAG "DataTransmit"

// 使编译器在其他地方搜索`recv_setpoint_t`的定义以便于在main中设置回调定义
extern void recv_setpoint(radio_packet_t *packet); // 接收的设定点数据
extern void send_on_slave(radio_packet_t *packet); // 当处于从机模式下的回传数据

void init_transmit()
{
    // 初始化无线传输
    init_radio();

    auto bt_CB = [](void) // 蓝牙控制器回调
    {
        radio_packet_t rp = {};
        auto cp = (CRTPPacket *)&rp.data;
        auto sp = (packet_setpoint_t *)cp->data;
        get_setpoint_data_from_controller(sp->raw);
        recv_setpoint(&rp);
    };

    // 注册接收回调
    if (CONFIG.radio_mode == BT_CONTROLLER)
        Controller.setCallBack(XBOX_ON_INPUT, bt_CB);
    else
        radio_set_port_callback(CRTP_PORT_SETPOINT, recv_setpoint);

    // 启动发送任务
    auto send_task = [](void *parameter)
    {
        static const TickType_t TRANSMIT_FQ = HZ2TICKS(80);    // 发送频率 HZ/S
        static TickType_t xLastWakeTime = xTaskGetTickCount(); // 最后唤醒时间

        if (CONFIG.control_mode == MASTER) // 发送设控制数据
            while (true)
            {
                radio_packet_t rp = {};
                auto cp = (CRTPPacket *)&rp.data;
                get_setpoint_data_from_controller(cp->data);
                radio_send_packet(&rp);

                vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TRANSMIT_FQ));
            }
        else if (CONFIG.control_mode == SLAVE) // 发送从机数据
            while (true)
            {
                radio_packet_t rp = {};
                send_on_slave(&rp);
                radio_send_packet(&rp);
                vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TRANSMIT_FQ));
            };
    };

    BaseType_t result =
        xTaskCreate(send_task, "DataTransmitTask", 4096, NULL, TP_N, NULL);
    if (result != pdPASS)
        ESP_LOGE(TAG, "Failed to create DataTransmitTask");
}
