#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <DNSServer.h>
#include <PubSubClient.h>
#include <LittleFS.h>

// Константы Wi-Fi по умолчанию (как в исходнике)
#ifndef STASSID
#define STASSID "hudnet"
#define STAPSK  "7950295932"
#endif

// === Модули ===
#include "pins.h"
#include "config.h"
#include "wifi_portal.h"
#include "web_ui.h"
#include "mqtt_ha.h"
#include "sensors.h"
#include "display.h"
#include "time_sync.h"
#include "melodies.h"

// --- Глобальные объекты инфраструктуры ---
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;
DNSServer dnsServer;
WiFiClient espClient;
PubSubClient mqtt(espClient);

// --- Глобальное состояние приложения ---
String client_name = "sensor_outside";
bool guard_mode = false; // управляется из MQTT/веба, играет мелодии

// Таймеры
static unsigned long t_now = 0;
static unsigned long t_fast = 0;   // 500 мс
static unsigned long t_temp = 0;   // 5 с
static unsigned long t_oled = 0;   // 200 мс

// Колбэк смены guard_mode (для mqtt_ha)
void onGuardChange(bool new_state) {
  if (guard_mode != new_state) {
    guard_mode = new_state;
    if (guard_mode) change_guard_mode_song();
    else            change_guard_mode_song2();
  }
}

void setup() {
  Serial.begin(115200);
  delay(50);

  // ФС
  LittleFS.begin();

  // Конфиг (загрузим; при первом запуске будут дефолты)
  AppConfig &cfg = appcfg();
  cfg.wifi_ssid = STASSID;
  cfg.wifi_pass = STAPSK;
  loadConfig(); // перезапишет из /config.json, если есть

  // OTA маршрут
  httpUpdater.setup(&server, "/update"); // без авторизации (как просили)

  // Инициализации модулей UI/портала
  wifi_portal::begin(server, dnsServer, cfg);
  web_ui::begin(server, cfg, client_name);
  wifi_portal::setupCaptiveRoutes(server);

  // Дисплей, датчики, время
  display::begin();
  sensors::begin();
  time_sync::beginGMT3();

  // Пытаемся подключиться к Wi-Fi; иначе откроем AP
  if (!wifi_portal::connectWiFiSTA(cfg)) {
    wifi_portal::startAPMode();
  }

  // MQTT (Discovery и работа начнётся только в STA)
  mqtt_ha::begin(mqtt, cfg, client_name, onGuardChange);

  // Первый “джингл”
  play_song();

  // Запустить HTTP сервер
  server.begin();
}

void loop() {
  t_now = millis();

  server.handleClient();
  if (wifi_portal::isApMode()) wifi_portal::dnsLoop();
  wifi_portal::ensureWifi(appcfg());

  // >>> было:
  // mqtt_ha::ensureMqtt(wifi_portal::isApMode());
  // mqtt.loop();

  // >>> стало:
  bool justConnected = mqtt_ha::ensureMqtt(wifi_portal::isApMode());
  mqtt.loop();

  // Если только что подключились — сразу даём первичную публикацию (retained)
  if (justConnected) {
    bool m=false, b=false; int l=0;
    sensors::readFast(m, b, l);
    mqtt_ha::publishFast(m, b, l);

    double tC = sensors::readTemperature();
    if (!isnan(tC)) mqtt_ha::publishTemp(tC);
  }

  // дальше всё как было: «по изменению»/таймеры
  if (t_now - t_fast >= 500) {
    t_fast = t_now;
    bool m=false, b=false; int l=0;
    sensors::readFast(m, b, l);
    mqtt_ha::publishFast(m, b, l);
  }

  static double lastTemp = NAN;
  if (t_now - t_temp >= 5000) {
    t_temp = t_now;
    double tC = sensors::readTemperature();
    if (!isnan(tC)) {
      lastTemp = tC;
      mqtt_ha::publishTemp(tC);
    }
  }

  if (t_now - t_oled >= 200) {
    t_oled = t_now;
    String hhmm = time_sync::timeHHMM();
    display::draw(lastTemp, hhmm, sensors::lastMotion(), guard_mode, sensors::lastButton());
  }
}
