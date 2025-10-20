#include "config.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

static AppConfig g_cfg;                 // единый экземпляр
static const char* CONFIG_PATH = "/config.json";

AppConfig& appcfg() { return g_cfg; }

bool loadConfig() {
  if (!LittleFS.exists(CONFIG_PATH)) return false;
  File f = LittleFS.open(CONFIG_PATH, "r");
  if (!f) return false;

  DynamicJsonDocument doc(768);
  auto err = deserializeJson(doc, f);
  f.close();
  if (err) return false;

  g_cfg.wifi_ssid = doc["wifi_ssid"] | g_cfg.wifi_ssid;
  g_cfg.wifi_pass = doc["wifi_pass"] | g_cfg.wifi_pass;
  g_cfg.host      = doc["host"]      | g_cfg.host;
  g_cfg.port      = doc["port"]      | g_cfg.port;
  g_cfg.user      = doc["user"]      | g_cfg.user;
  g_cfg.pass      = doc["pass"]      | g_cfg.pass;
  g_cfg.client_id = doc["client_id"] | g_cfg.client_id;
  return true;
}

bool saveConfig() {
  DynamicJsonDocument doc(768);
  doc["wifi_ssid"] = g_cfg.wifi_ssid;
  doc["wifi_pass"] = g_cfg.wifi_pass;
  doc["host"]      = g_cfg.host;
  doc["port"]      = g_cfg.port;
  doc["user"]      = g_cfg.user;
  doc["pass"]      = g_cfg.pass;
  doc["client_id"] = g_cfg.client_id;

  File f = LittleFS.open(CONFIG_PATH, "w");
  if (!f) return false;
  serializeJson(doc, f);
  f.close();
  return true;
}
