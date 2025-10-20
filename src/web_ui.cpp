#include "web_ui.h"
#include "wifi_portal.h"
#include <ESP8266WiFi.h>
#include <LittleFS.h>

static AppConfig* s_cfg = nullptr;
static ESP8266WebServer* s_server = nullptr;
static String s_client_name;

static String htmlEscape(String s) {
  s.replace("&", "&amp;");
  s.replace("<", "&lt;");
  s.replace(">", "&gt;");
  s.replace("\"", "&quot;");
  s.replace("'", "&#39;");
  return s;
}

static String buildNetworksOptions() {
  String html;
  int n = WiFi.scanNetworks();
  if (n <= 0) {
    html += "<option disabled>Нет сетей поблизости</option>";
  } else {
    std::vector<String> seen;
    seen.reserve(n);
    for (int i = 0; i < n; i++) {
      String ssid = WiFi.SSID(i);
      bool dup = false;
      for (auto &x : seen) { if (x == ssid) { dup = true; break; } }
      if (dup) continue;
      seen.push_back(ssid);

      int rssi = WiFi.RSSI(i);
      bool secured = (WiFi.encryptionType(i) != ENC_TYPE_NONE);
      String label = htmlEscape(ssid) + " (" + String(rssi) + " dBm" + (secured ? ", 🔒" : ", open") + ")";
      html += "<option value=\"" + htmlEscape(ssid) + "\">" + label + "</option>";
    }
  }
  WiFi.scanDelete();
  return html;
}

static String chipId() {
  char buf[10];
  snprintf(buf, sizeof(buf), "%06X", ESP.getChipId());
  return String(buf);
}

static String renderPage() {
  String id = s_cfg->client_id.length() ? s_cfg->client_id : (s_client_name + "-" + chipId());
  String wifiInfo = wifi_portal::isApMode()
    ? "Mode: <b>AP</b> (connect: <code>ESP-" + chipId() + "</code>) • IP: " + WiFi.softAPIP().toString()
    : "Mode: <b>STA</b> • IP: " + WiFi.localIP().toString() + " • RSSI: " + String(WiFi.RSSI()) + " dBm";

  String options = buildNetworksOptions();

  String page =
    "<!DOCTYPE html><html><head><meta charset='utf-8'/>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'/>"
    "<title>ESP8266 Setup</title>"
    "<style>body{font-family:system-ui,Arial,sans-serif;max-width:720px;margin:24px auto;padding:0 12px}"
    "label{display:block;margin-top:12px}input,select{width:100%;padding:8px;margin-top:6px}"
    "button{margin-top:16px;padding:10px 16px;cursor:pointer}"
    ".card{border:1px solid #ddd;padding:16px;border-radius:12px;box-shadow:0 2px 6px rgba(0,0,0,.06)}"
    ".row{display:grid;grid-template-columns:1fr 1fr;gap:12px}a.button{display:inline-block;margin-left:8px;text-decoration:none;border:1px solid #ddd;padding:8px 12px;border-radius:8px}"
    ".hint{color:#666;font-size:12px;margin-top:4px}"
    "</style>"
    "</head><body><h2>ESP8266 • " + htmlEscape(s_client_name) + "</h2>"
    "<p>" + wifiInfo + " • ChipID: " + chipId() + "</p>"
    "<div class='card'>"
    "<form method='POST' action='/save'>"
    "<h3>Wi-Fi</h3>"
    "<label>Выберите сеть"
    "<select id='ssid_select' onchange='document.getElementsByName(\"wifi_ssid\")[0].value=this.value'>"
    "<option value='' disabled selected>— выбрать SSID —</option>" + options +
    "</select></label>"
    "<label>SSID<input name='wifi_ssid' value='" + htmlEscape(s_cfg->wifi_ssid) + "'></label>"
    "<label>Password<input name='wifi_pass' type='password' value='" + htmlEscape(s_cfg->wifi_pass) + "'></label>"
    "<div class='row'><div><h3>MQTT</h3>"
    "<label>Host/IP<input name='host' value='" + htmlEscape(s_cfg->host) + "'></label>"
    "<label>Port<input name='port' type='number' value='" + String(s_cfg->port) + "'></label>"
    "<label>Username<input name='user' value='" + htmlEscape(s_cfg->user) + "'></label>"
    "<label>Password<input name='pass' type='password' value='" + htmlEscape(s_cfg->pass) + "'></label>"
    "<label>Client ID<input name='client_id' value='" + htmlEscape(id) + "'></label>"
    "</div></div>"
    "<div class='hint'>В режиме AP после сохранения устройство перезагрузится и попробует подключиться к Wi-Fi.</div>"
    "<button type='submit'>Save & Reboot</button>"
    "<a class='button' href='/update'>Firmware Update</a>"
    "<a class='button' href='/reboot'>Reboot</a>"
    "</form>"
    "</div>"
    "</body></html>";
  return page;
}

static void handleRoot() {
  s_server->send(200, "text/html; charset=utf-8", renderPage());
}

static void handleSave() {
  if (s_server->method() != HTTP_POST) { s_server->send(405, "text/plain", "Method Not Allowed"); return; }
  if (s_server->hasArg("wifi_ssid")) s_cfg->wifi_ssid = s_server->arg("wifi_ssid");
  if (s_server->hasArg("wifi_pass")) s_cfg->wifi_pass = s_server->arg("wifi_pass");
  if (s_server->hasArg("host"))      s_cfg->host      = s_server->arg("host");
  if (s_server->hasArg("port"))      s_cfg->port      = (uint16_t)s_server->arg("port").toInt();
  if (s_server->hasArg("user"))      s_cfg->user      = s_server->arg("user");
  if (s_server->hasArg("pass"))      s_cfg->pass      = s_server->arg("pass");
  if (s_server->hasArg("client_id")) s_cfg->client_id = s_server->arg("client_id");

  saveConfig();
  s_server->send(200, "text/plain", "Saved. Rebooting in 1s...");
  delay(1000);
  ESP.restart();
}

static void handleReboot() {
  s_server->send(200, "text/plain", "Rebooting...");
  delay(200);
  ESP.restart();
}

namespace web_ui {
  void begin(ESP8266WebServer& server, AppConfig& cfg, const String& client_name) {
    s_server = &server;
    s_cfg = &cfg;
    s_client_name = client_name;

    server.on("/", handleRoot);
    server.on("/save", handleSave);
    server.on("/reboot", handleReboot);
  }
}
