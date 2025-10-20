#pragma once
#include <Arduino.h>

struct AppConfig {
  // Wi-Fi
  String wifi_ssid;
  String wifi_pass;

  // MQTT
  String host = "192.168.18.1";
  uint16_t port = 1883;
  String user = "";
  String pass = "";
  String client_id = ""; // если пусто — генерируем из ChipID
};

// Глобальный доступ к конфигу (внутри .cpp объявлен статически)
AppConfig& appcfg();

// Работа с LittleFS:/config.json
bool loadConfig();
bool saveConfig();
