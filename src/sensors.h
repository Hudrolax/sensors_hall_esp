#pragma once
#include <Arduino.h>

namespace sensors {
  void begin();

  // Быстрые сенсоры (PIR, кнопка, свет)
  void readFast(bool& motion, bool& button, int& light);

  // Температура DS18B20 (°C). Возвращает NAN при ошибке.
  double readTemperature();

  // Последние значения для отображения
  bool lastMotion();
  bool lastButton();
}
