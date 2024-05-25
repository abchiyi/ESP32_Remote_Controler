#include <Arduino.h>

typedef void (*set_storage)(int *);

template <typename T>
struct sCongfig
{
  int addr_start;
  uint8_t size;
  bool changed;
  T &ref;
  sCongfig(T &value) : ref(value)
  {
    size = sizeof(this->ref);
  }
};

class storage_config
{

private:
  uint16_t addr_start = 0; // 储存起始地址
public:
  void begin();

  void save_all();

  template <typename T>
  sCongfig<T> create_sconfig(T &value);

  template <typename T>
  void read(sCongfig<T> &config);

  template <typename T>
  void write(sCongfig<T> &config);
};

extern storage_config Storage_config;