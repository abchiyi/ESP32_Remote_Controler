#include <tool.h>

void nvs_call(const char *name_space, std::function<void(Preferences &)> cb_fn)
{
  Preferences PREFS;
  PREFS.begin(name_space);
  cb_fn(PREFS);
  PREFS.end();
}

IRAM_ATTR uint8_t calculate_cksum(void *data, size_t len)
{
  auto c = (unsigned char *)data;
  unsigned char cksum = 0;
  for (int i = 0; i < len; i++)
    cksum += *(c++);
  return cksum;
}
