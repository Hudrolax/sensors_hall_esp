#pragma once
#include <Arduino.h>

namespace time_sync {
  void beginGMT3();       // NTP + GMT+3
  String timeHHMM();      // локальное время HH:MM
}
