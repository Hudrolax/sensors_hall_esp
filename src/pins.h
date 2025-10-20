#pragma once
#include <Arduino.h>

// Пины — как в исходном проекте, НЕ МЕНЯЕМ
#define ONE_WIRE_BUS   4    // GPIO4 (D2)
#define HC_SR501_PIN   5    // GPIO5 (D1)
#define ANALOG_IN_PIN  A0   // ADC
#define BUTTON_PIN     14   // GPIO14 (D5)
#define BUZZER_PIN     15   // GPIO15 (D8)

// I2C для OLED (SH1106) — как в исходнике: Wire.begin(0,2)
#define I2C_SDA_PIN    0    // GPIO0 (D3)
#define I2C_SCL_PIN    2    // GPIO2 (D4)
