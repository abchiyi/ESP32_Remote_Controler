#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <Arduino.h>

typedef enum
{
  btnA,
  btnB,
  btnX,
  btnY,
  btnLB,
  btnRB,
  btnSelect,
  btnStart,
  btnXbox,
  btnLS,
  btnRS,
  btnShare,
  btnDirUp,
  btnDirRight,
  btnDirDown,
  btnDirLeft
} XBOX_BUTTON;

typedef enum
{
  joyLHori,
  joyLVert,
  joyRHori,
  joyRVert,
  trigLT,
  trigRT
} XBOX_ANALOG_HAT;

struct xbox_control_data
{
  bool button_bits[16];  // bool
  int16_t analog_hat[6]; // 0 ~ 2047
};

class CONTROLLER
{
private:
public:
  bool connected;
  bool button_bits[16];  // bool
  int16_t analog_hat[6]; // 0 ~ 2047

  void begin();
  bool getButtonPress(XBOX_BUTTON);
  int16_t getAnalogHat(XBOX_ANALOG_HAT);
};

extern CONTROLLER Controller;

#endif
