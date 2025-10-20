#pragma once
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include "config.h"

namespace wifi_portal {

// Инициализация внутренних состояний (ссылки на server/dns не храним)
void begin(ESP8266WebServer& server, DNSServer& dns, AppConfig& cfg);

// Попытка подключения в STA (true при успехе)
bool connectWiFiSTA(AppConfig& cfg, unsigned long timeout_ms = 15000);

// Включить AP-режим (открытая сеть ESP-<chipid>)
void startAPMode();

// Поддержание подключения (STA↔AP failover)
void ensureWifi(AppConfig& cfg);

// Каптив-маршруты и обработка DNS
void setupCaptiveRoutes(ESP8266WebServer& server);
void dnsLoop();

// Статус AP
bool isApMode();

}
