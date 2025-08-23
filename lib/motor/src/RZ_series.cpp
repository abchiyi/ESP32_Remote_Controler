
#include "RZ_series.h"

RZ_HBridgeDriver::RZ_HBridgeDriver(uint8_t fi, uint8_t bi, uint8_t ch_fw, uint8_t ch_bw)
{
    this->pin_forward = fi;
    this->pin_backward = bi;
    this->channel_forward = ch_fw;
    this->channel_backward = ch_bw;
    
    // 设置引脚为输出模式
    pinMode(pin_forward, OUTPUT);
    pinMode(pin_backward, OUTPUT);
    
    // 初始化LEDC
    setupLEDC();
}

void RZ_HBridgeDriver::setupLEDC()
{
    // 配置LEDC通道
    ledcSetup(channel_forward, LEDC_FREQ, LEDC_RESOLUTION);
    ledcSetup(channel_backward, LEDC_FREQ, LEDC_RESOLUTION);
    
    // 将通道附加到GPIO引脚
    ledcAttachPin(pin_forward, channel_forward);
    ledcAttachPin(pin_backward, channel_backward);
}

void RZ_HBridgeDriver::forward(uint8_t speed)
{
    ledcWrite(channel_forward, speed);
    ledcWrite(channel_backward, 0);
}

void RZ_HBridgeDriver::backward(uint8_t speed)
{
    ledcWrite(channel_backward, speed);
    ledcWrite(channel_forward, 0);
}

void RZ_HBridgeDriver::stop()
{
    ledcWrite(channel_forward, 255);
    ledcWrite(channel_backward, 255);
}

void RZ_HBridgeDriver::free()
{
    ledcWrite(channel_forward, 0);
    ledcWrite(channel_backward, 0);
}
