#include <Arduino.h>
#include <XBOXONE.h>

struct ControllerStatus
{
  bool btnA, btnB, btnX, btnY;
  bool btnShare, btnStart, btnSelect, btnXbox;
  bool btnLB, btnRB;
  bool btnLS, btnRS;
  bool btnDirUp, btnDirLeft, btnDirRight, btnDirDown;
  int16_t joyLHori;
  int16_t joyLVert;
  int16_t joyRHori;
  int16_t joyRVert;
  int16_t trigLT, trigRT;
};

class Controller
{
private:
  /* data */
  void setPointer();
  bool *Connected;

public:
  ControllerStatus data;
  bool getConnectStatus();
  String toString();
  void begin();
};

extern XBOXONE Xbox;