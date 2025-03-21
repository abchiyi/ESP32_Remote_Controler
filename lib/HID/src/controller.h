#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <Arduino.h>
#include <XBOXONE.h>

class CONTROLLER
{
public:
  bool btnA, btnB, btnX, btnY;
  bool btnShare, btnStart, btnSelect, btnXbox;
  bool btnLB, btnRB;
  bool btnLS, btnRS;
  bool btnDirUp, btnDirLeft, btnDirRight, btnDirDown;
  // 0 ~ 2047
  int16_t joyLHori;
  int16_t joyLVert;
  int16_t joyRHori;
  int16_t joyRVert;
  int16_t trigLT, trigRT;

  bool connected;

  String toString();
  void update();
  void begin();
};

extern CONTROLLER Controller;

#endif
