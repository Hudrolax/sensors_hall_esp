#pragma once
#include <ESP8266WebServer.h>
#include "config.h"

// Веб-страница настроек (Wi-Fi + MQTT), /save, /reboot
namespace web_ui {
  void begin(ESP8266WebServer& server, AppConfig& cfg, const String& client_name);
}
