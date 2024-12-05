#include "U8g2lib.h"
#pragma once

struct ELEMENT
{

protected:
  float __x = 0;
  float __y = 0;
  float __width = 0;
  float __height = 0;
  float __min_width = 0;
  float __min_height = 0;

public:
  float x = 0;
  float y = 0;
  float width = 0;
  float height = 0;
  float min_width = 0;
  float min_height = 0;

  virtual void draw(U8G2 &u8g2) = 0;
  int transition = 0;

  // 渐近动画
  void Transition(float *a, float *a_trg, uint8_t n)
  {
    if (!n)
      *a = *a_trg;

    if (*a != *a_trg)
    {
      if (fabs(*a - *a_trg) < 0.15f)
        *a = *a_trg;
      else
        *a += (*a_trg - *a) / (n / 10.0f);
    }
  }
};

struct BOX : ELEMENT
{
  float round_corner = 0;

  void draw(U8G2 &u8g2)
  {
    Transition(&__x, &x, transition);
    Transition(&__y, &y, transition);

    Transition(&__width, &width, transition);
    Transition(&__height, &height, transition);

    Transition(&__min_width, &min_width, transition);
    Transition(&__min_height, &min_height, transition);

    if (round_corner)
      u8g2.drawRBox(__x, __y,
                    __width < __min_width ? __min_width : __width,
                    __height < __min_height ? __min_height : __height,
                    round_corner);
    else
      u8g2.drawBox(__x, __y,
                   __width < __min_width ? __min_width : __width,
                   __height < __min_height ? __min_height : __height);
  };
};
