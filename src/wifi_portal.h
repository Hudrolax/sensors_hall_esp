#pragma once
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include "config.h"

namespace wifi_portal {

// Инициализация Wi-Fi портала/слежения за состоянием.
void begin(ESP8266WebServer& server, DNSServer& dns, AppConfig& cfg);

// Одноразовая попытка быстрого подключения к STA (обычно из setup()).
// Возвращает true при успешном подключении за timeout_ms.
bool connectWiFiSTA(AppConfig& cfg, unsigned long timeout_ms = 8000);

// Поднять точку доступа (работаем в WIFI_AP_STA, чтобы продолжать реконнект STA).
void startAPMode();

// Остановить точку доступа и вернуть чистый STA.
void stopAPMode();

// Периодически вызывать в loop(): управляет AP↔STA, включает AP после долгого оффлайна и гасит AP после устойчивого онлайна.
void ensureWifi(AppConfig& cfg);

// Зарегистрировать маршруты каптив-портала (редирект на /).
void setupCaptiveRoutes(ESP8266WebServer& server);

// В loop() обрабатывать DNS, когда поднят AP.
void dnsLoop();

// Признак поднятого SoftAP.
bool isApMode();

// Признак онлайна по STA (WiFi.status()==WL_CONNECTED).
bool online();

} // namespace wifi_portal
