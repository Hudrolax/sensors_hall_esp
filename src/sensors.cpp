#include "sensors.h"
#include "pins.h"
#include <OneWire.h>
#include <DallasTemperature.h>

static bool s_motion = false;
static bool s_button = false;
static int  s_light  = 0;

static OneWire oneWire(ONE_WIRE_BUS);
static DallasTemperature dallas(&oneWire);

// Адрес из исходного проекта
static DeviceAddress addr = {0x28, 0xFF, 0x95, 0x7F, 0x93, 0x16, 0x04, 0xF4};

namespace sensors {

void begin() {
  pinMode(BUZZER_PIN, OUTPUT);     // зуммер
  pinMode(HC_SR501_PIN, INPUT);    // PIR
  pinMode(BUTTON_PIN, INPUT);      // кнопка (как в исходнике, без PULLUP)

  dallas.begin();
  dallas.setResolution(addr, 12);
}

void readFast(bool& motion, bool& button, int& light) {
  s_motion = digitalRead(HC_SR501_PIN);
  s_button = digitalRead(BUTTON_PIN);
  s_light  = analogRead(ANALOG_IN_PIN);

  motion = s_motion;
  button = s_button;
  light  = s_light;
}

double readTemperature() {
  dallas.requestTemperaturesByAddress(addr);
  double tC = dallas.getTempC(addr);
  if (tC == DEVICE_DISCONNECTED_C || tC == -127 || tC == 85) return NAN;
  return tC;
}

bool lastMotion() { return s_motion; }
bool lastButton() { return s_button; }

}
