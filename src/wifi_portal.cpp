#include "wifi_portal.h"
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>

namespace wifi_portal {

static bool               s_ap_mode = false;
static unsigned long      s_last_wifi_ok = 0;
static DNSServer*         s_dns = nullptr;
static ESP8266WebServer*  s_server = nullptr;

// Тайминги поведения
static const unsigned long START_AP_AFTER_OFFLINE_MS = 20000; // через 20s оффлайна поднимаем AP
static const unsigned long STOP_AP_AFTER_ONLINE_MS   = 5000;  // через 5s стабильного онлайна гасим AP

// Вспомогательные
static void ensureDnsStarted() {
  if (!s_dns) return;
  // Привязываем DNS к текущему IP softAP
  s_dns->setTTL(30);
  if (!s_dns->start(53, "*", WiFi.softAPIP())) {
    s_dns->stop();
    s_dns->start(53, "*", WiFi.softAPIP());
  }
}

static void ensureDnsStopped() {
  if (s_dns) s_dns->stop();
}

static void handleCaptive(ESP8266WebServer& server) {
  IPAddress ip = s_ap_mode ? WiFi.softAPIP() : WiFi.localIP();
  String url = String("http://") + ip.toString() + "/";
  server.sendHeader("Location", url, true);
  server.send(302, "text/plain", "");
}

// === API ===

void begin(ESP8266WebServer& server, DNSServer& dns, AppConfig& /*cfg*/) {
  s_server = &server;
  s_dns    = &dns;

  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.mode(WIFI_STA);   // стартуем в STA

  if (WiFi.status() == WL_CONNECTED) {
    s_last_wifi_ok = millis();
  } else {
    // если стартуем без связи — дадим 20s «окна» до поднятия AP
    s_last_wifi_ok = millis();
  }
}

bool connectWiFiSTA(AppConfig& cfg, unsigned long timeout_ms) {
  WiFi.mode(WIFI_STA);
  if (cfg.wifi_ssid.length()) WiFi.begin(cfg.wifi_ssid.c_str(), cfg.wifi_pass.c_str());

  unsigned long until = millis() + timeout_ms;
  while (millis() < until) {
    if (WiFi.status() == WL_CONNECTED) {
      s_last_wifi_ok = millis();
      return true;
    }
    delay(100);
    yield();
  }
  return false;
}

void startAPMode() {
  if (s_ap_mode) return;

  WiFi.mode(WIFI_AP_STA); // важно: держим STA активным параллельно
  WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));

  // SSID вида ESP-<CHIPID>, пароль пустой => открытая сеть
  const String ssid = String("ESP-") + String(ESP.getChipId(), HEX);
  WiFi.softAP(ssid, "", 1, false, 4);

  ensureDnsStarted();
  s_ap_mode = true;
}

void stopAPMode() {
  if (!s_ap_mode) return;
  ensureDnsStopped();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  s_ap_mode = false;
}

void ensureWifi(AppConfig& cfg) {
  const unsigned long now = millis();

  // Если STA подключен — фиксируем онлайн и по таймеру гасим AP
  if (WiFi.status() == WL_CONNECTED) {
    static unsigned long stable_since = 0;
    s_last_wifi_ok = now;

    if (s_ap_mode) {
      if (!stable_since) stable_since = now;
      if (now - stable_since >= STOP_AP_AFTER_ONLINE_MS) {
        stopAPMode();
        stable_since = 0;
      }
    } else {
      stable_since = 0;
    }
    return;
  }

  // Не онлайн: неблокирующие попытки реконнекта и, если долго — поднимаем AP
  static unsigned long s_last_try_ms = 0;
  if (cfg.wifi_ssid.length() && (now - s_last_try_ms >= 2000)) {
    // повторная попытка с теми же кредами
    WiFi.mode(WIFI_STA);
    WiFi.begin(cfg.wifi_ssid.c_str(), cfg.wifi_pass.c_str());
    s_last_try_ms = now;
  }

  if (!s_ap_mode && (now - s_last_wifi_ok > START_AP_AFTER_OFFLINE_MS)) {
    startAPMode();
  }

  if (s_ap_mode) {
    // DNS должен обслуживать каптив-портал
    ensureDnsStarted();
  }
}

void setupCaptiveRoutes(ESP8266WebServer& server) {
  // Маршруты, которые используют ОС для детекта портала
  server.on("/generate_204", HTTP_ANY, [&server]{ handleCaptive(server); }); // Android
  server.on("/hotspot-detect.html", HTTP_ANY, [&server]{ handleCaptive(server); }); // Apple
  server.on("/ncsi.txt", HTTP_ANY, [&server]{ handleCaptive(server); }); // Windows
  // Общий редирект
  server.onNotFound([&server]{ handleCaptive(server); });
}

void dnsLoop() {
  if (s_ap_mode && s_dns) s_dns->processNextRequest();
}

bool isApMode() {
  return s_ap_mode;
}

bool online() {
  return WiFi.status() == WL_CONNECTED;
}

} // namespace wifi_portal
