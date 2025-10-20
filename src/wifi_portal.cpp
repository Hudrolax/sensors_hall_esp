#include "wifi_portal.h"
#include <ESP8266WiFi.h>
#include <Arduino.h>

static bool s_ap_mode = false;
static unsigned long s_last_wifi_ok = 0;
static DNSServer* s_dns_ptr = nullptr;

static String chipId() {
  char buf[10];
  snprintf(buf, sizeof(buf), "%06X", ESP.getChipId());
  return String(buf);
}

namespace wifi_portal {

void begin(ESP8266WebServer& server, DNSServer& dns, AppConfig& cfg) {
  (void)server; (void)cfg;
  s_dns_ptr = &dns;
  s_last_wifi_ok = millis();
}

bool connectWiFiSTA(AppConfig& cfg, unsigned long timeout_ms) {
  s_ap_mode = false;
  WiFi.mode(WIFI_STA);
  WiFi.begin(cfg.wifi_ssid.c_str(), cfg.wifi_pass.c_str());
  Serial.printf("[WiFi] Connecting to '%s' ...\n", cfg.wifi_ssid.c_str());

  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - t0) < timeout_ms) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[WiFi] Connected, IP: %s, RSSI: %d dBm\n",
                  WiFi.localIP().toString().c_str(), WiFi.RSSI());
    s_last_wifi_ok = millis();
    return true;
  }
  Serial.println("[WiFi] Failed to connect");
  return false;
}

void startAPMode() {
  s_ap_mode = true;
  WiFi.mode(WIFI_AP_STA); // AP + возможность сканировать
  String ap_ssid = "ESP-" + chipId();
  WiFi.softAP(ap_ssid.c_str()); // открытый AP, без пароля
  delay(100);

  if (s_dns_ptr) {
    s_dns_ptr->setErrorReplyCode(DNSReplyCode::NoError);
    s_dns_ptr->start(53, "*", WiFi.softAPIP());
  }
  Serial.printf("[AP] SSID: %s  IP: %s\n", ap_ssid.c_str(), WiFi.softAPIP().toString().c_str());
}

void ensureWifi(AppConfig& cfg) {
  if (WiFi.status() == WL_CONNECTED) {
    s_last_wifi_ok = millis();
    return;
  }
  if (!s_ap_mode) {
    if (!connectWiFiSTA(cfg, 8000)) {
      if (millis() - s_last_wifi_ok > 15000) {
        startAPMode();
      }
    }
  }
}

static void handleCaptivePortal(ESP8266WebServer& server) {
  String url = String("http://") + (s_ap_mode ? WiFi.softAPIP().toString() : WiFi.localIP().toString()) + "/";
  server.sendHeader("Location", url, true);
  server.send(302, "text/plain", "");
}

void setupCaptiveRoutes(ESP8266WebServer& server) {
  server.on("/generate_204", [&](){ handleCaptivePortal(server); });
  server.on("/gen_204",      [&](){ handleCaptivePortal(server); });
  server.on("/hotspot-detect.html", [&](){ handleCaptivePortal(server); });
  server.on("/ncsi.txt",     [&](){ handleCaptivePortal(server); });
  server.on("/fwlink",       [&](){ handleCaptivePortal(server); });
  server.on("/favicon.ico",  [&](){ handleCaptivePortal(server); });

  server.onNotFound([&](){
    if (s_ap_mode) handleCaptivePortal(server);
    else server.send(404, "text/plain", "Not found");
  });
}

void dnsLoop() {
  if (s_dns_ptr) s_dns_ptr->processNextRequest();
}

bool isApMode() { return s_ap_mode; }

}
