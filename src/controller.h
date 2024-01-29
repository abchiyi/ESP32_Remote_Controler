#include <Arduino.h>

class Controller
{
private:
  /* data */
  static const uint16_t maxJoy = 0xffff;
  void setPointer();
  bool *Connected;

public:
  bool btnA, btnB, btnX, btnY;
  bool btnShare, btnStart, btnSelect, btnXbox;
  bool btnLB, btnRB;
  bool btnLS, btnRS;
  bool btnDirUp, btnDirLeft, btnDirRight, btnDirDown;
  int16_t joyLHori = maxJoy / 2;
  int16_t joyLVert = maxJoy / 2;
  int16_t joyRHori = maxJoy / 2;
  int16_t joyRVert = maxJoy / 2;
  int16_t trigLT, trigRT;

  bool getConnectStatus();
  String toString();
  void begin();
};
