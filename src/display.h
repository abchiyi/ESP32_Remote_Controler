
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <radio.h>

class Display
{
private:
  // 主显示器 Spi：
  uint8_t DO = 14;
  uint8_t DI = 13;
  uint8_t DC = 26;
  uint8_t RES = 25;
  uint8_t CS = 27;

  // 副显示器 I2C
  uint8_t SCK = 22;
  uint8_t SDA = 21;

  Adafruit_SSD1306 displayMain;
  Adafruit_SSD1306 displaySub;

public:
  void begin(Radio *radio);
};
