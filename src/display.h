#pragma once
#include <Arduino.h>

namespace display {
  void begin();
  void draw(double tempC, const String& time_str, bool move, bool guard_mode, bool btn);
}
