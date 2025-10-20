#include "display.h"
#include "pins.h"
#include <U8g2lib.h>
#include <Wire.h>

static U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

namespace display {

void begin() {
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN); // как в исходнике: (0,2)
  u8g2.begin();
}

void draw(double tempC, const String& time_str, bool move, bool guard_mode, bool btn) {
  u8g2.clearBuffer();

  // Температура
  u8g2.setFont(u8g2_font_ncenB18_tf);
  if (isnan(tempC)) {
    u8g2.setCursor(0, 20);
    u8g2.print("--");
  } else {
    u8g2.setCursor(0, 20);
    u8g2.print(String(tempC, 1));
  }
  u8g2.setFont(u8g2_font_ncenB08_tf);
  u8g2.setCursor(62, 8);
  u8g2.print("C");

  // Время
  u8g2.setFont(u8g2_font_ncenB18_tf);
  u8g2.setCursor(10, 50);
  u8g2.print(time_str);

  // Движение
  u8g2.setFont(u8g2_font_ncenB08_tf);
  u8g2.setCursor(0, 62);
  u8g2.print(move ? "move" : "");

  // Guard
  u8g2.setCursor(60, 62);
  u8g2.print(guard_mode ? "guard mode" : "");

  // Кнопка
  u8g2.setFont(u8g2_font_ncenB12_tf);
  u8g2.setCursor(100, 12);
  u8g2.print(btn ? "B" : "");

  u8g2.sendBuffer();
}

}
