#include <Arduino.h>

template <typename T>
struct sCongfig
{
  uint16_t addr_start;
  uint8_t size;
  T &ref;
  sCongfig(T &value) : ref(value)
  {
    size = sizeof(this->ref);
  }
};

void save_all();

void read_all();
