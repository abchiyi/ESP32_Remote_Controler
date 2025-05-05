#ifndef BAT_H
#define BAT_H
#include "Arduino.h"
#include "functional"

void init_bat();

typedef std::function<void(void)> bat_cbfn_t;
typedef enum
{
    ON_LOW_BATTERY,
} bat_irq_t;

uint32_t
get_bat_mv();

uint8_t get_bat_cell();

uint8_t get_battery_percentage();

uint8_t set_bat_callback(bat_irq_t, bat_cbfn_t);

#endif
