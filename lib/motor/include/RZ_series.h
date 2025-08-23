#ifndef RZ_series_h
#define RZ_series_h

#include "Arduino.h"
#include "motor.h"

// ESP32 LEDC 参数
#define LEDC_FREQ       5000    // PWM频率 5kHz
#define LEDC_RESOLUTION 8       // 8位分辨率 (0-255)
#define LEDC_CHANNEL_FW 0       // 前进通道
#define LEDC_CHANNEL_BW 1       // 后退通道

class RZ_HBridgeDriver : public motor
{
private:
    uint8_t pin_forward, pin_backward;
    uint8_t channel_forward, channel_backward;
    
public:
    RZ_HBridgeDriver(uint8_t fi, uint8_t bi, uint8_t ch_fw = LEDC_CHANNEL_FW, uint8_t ch_bw = LEDC_CHANNEL_BW);
    // ~RZ_series();
    
    void setupLEDC();
    void forward(uint8_t) override;
    void backward(uint8_t) override;
    void stop() override;
    void free() override;
};

#endif
