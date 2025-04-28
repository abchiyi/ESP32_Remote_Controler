#ifndef BAT_H
#define BAT_H
#include "Arduino.h"

void init_bat();

uint32_t get_bat_mv();

uint8_t get_bat_cell();

#endif
