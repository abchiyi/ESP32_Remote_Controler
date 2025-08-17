#include "bt_controller_link.h"

#include "XBOX.h"
#include "tool.h"
#include "radio.h"

#define TAG "bt_controller_link"

static QueueHandle_t crtpPacketDelivery = nullptr; // 数据分发队列

void xbox_on_input_cb()
{
    radio_packet_t rp = {};
    auto cp = (CRTPPacket *)&rp.data;
    auto sp = (packet_setpoint_t *)cp->data;

    get_setpoint_data_from_controller(sp->raw);

    xQueueSend(crtpPacketDelivery, &rp, 0);
}

class BtControllerLink : public RadioLink
{
public:
    BtControllerLink() = default;
    virtual ~BtControllerLink() = default;

    // 无需向蓝牙控制器发送任何数据
    esp_err_t send(radio_packet_t *rp) override
    {
        return ESP_OK;
    }

    esp_err_t recv(radio_packet_t *rp) override
    {
        xQueueReceive(crtpPacketDelivery, rp, portMAX_DELAY);
        return ESP_OK;
    }

    bool is_connected() override
    {
        return Controller.is_connected();
    }

    esp_err_t start() override
    {
        // 创建数据分发队列
        crtpPacketDelivery = xQueueCreate(10, sizeof(radio_packet_t::data));
        configASSERT(crtpPacketDelivery != nullptr);

        // 启动蓝牙手柄相关程序，并向 INPUT 事件注册回调以在有输入时将数据发送到队列
        Controller.begin();
        Controller.setCallBack(XBOX_ON_INPUT, xbox_on_input_cb);

        return ESP_OK;
    }

    // TODO 需要实现对无线连接的重置功能
    esp_err_t rest() override
    {
        return ESP_OK;
    }
};

RadioLink *bt_Controller_link()
{
    return (RadioLink *)new BtControllerLink();
}
