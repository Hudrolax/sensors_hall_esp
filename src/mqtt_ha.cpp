#include "mqtt_ha.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>

static PubSubClient* s_mqtt = nullptr;
static AppConfig* s_cfg = nullptr;
static String s_client_name;
static mqtt_ha::GuardChangeCb s_guard_cb = nullptr;

// Топики данных
static String t_status, t_motion, t_button, t_light, t_temp, t_guard_state, t_guard_set;
// Топики discovery
static String d_motion, d_button, d_light, d_temp, d_guard;

// Состояние
static bool  s_connected_session = false;   // новый MQTT-сессия после connect
static bool  s_guard = false;               // текущее состояние сторожа

// Последние опубликованные значения (для "publish-on-change")
static int   s_last_motion = 2;             // 2 = неинициализировано (булево)
static int   s_last_button = 2;             // 2 = неинициализировано (булево)
static int   s_last_light  = -32768;        // "очень давно" для аналога
static double s_last_temp  = NAN;
static unsigned long s_last_temp_pub_ms = 0;

// Порог для аналога и температуры
static const int    LIGHT_DELTA = 5;        // публиковать, если |Δ| >= 5
static const double TEMP_DELTA  = 0.2;      // публиковать, если |Δ| >= 0.2 °C
static const unsigned long TEMP_MAX_AGE_MS = 5UL * 60UL * 1000UL; // или раз в 5 минут

// ------------------------ helpers ------------------------
static String chipId() {
  char buf[10];
  snprintf(buf, sizeof(buf), "%06X", ESP.getChipId());
  return String(buf);
}

static void ensureTopics() {
  // База данных — ветка "home/<client_name>/..."
  const String base = "home/" + s_client_name + "/";

  t_status      = base + "status";
  t_motion      = base + "motion/state";
  t_button      = base + "button/state";
  t_light       = base + "light/state";
  t_temp        = base + "temperature/state";
  t_guard_state = base + "guard/state";
  t_guard_set   = base + "guard/set";

  const String node = s_client_name + "-" + chipId();
  d_motion = "homeassistant/binary_sensor/" + node + "/motion/config";
  d_button = "homeassistant/binary_sensor/" + node + "/button/config";
  d_light  = "homeassistant/sensor/"        + node + "/light/config";
  d_temp   = "homeassistant/sensor/"        + node + "/temperature/config";
  d_guard  = "homeassistant/switch/"        + node + "/guard/config";
}

static void publishDiscoveryOne(const String& topic, std::function<void(StaticJsonDocument<512>&)> fill) {
  StaticJsonDocument<512> d;
  fill(d);
  String out; serializeJson(d, out);
  bool ok = s_mqtt->publish(topic.c_str(), out.c_str(), true);
  if (!ok) {
    Serial.printf("[MQTT] DISCOVERY publish FAILED, topic=%s, len=%u (increase buffer?)\n",
                  topic.c_str(), out.length());
  }
}

// Корректный блок device для HA discovery
static void addDeviceBlock(StaticJsonDocument<512>& d) {
  JsonObject device = d.createNestedObject("device");
  device["identifiers"]  = "esp8266_" + chipId();
  device["manufacturer"] = "Custom";
  device["model"]        = "ESP8266 Sensor";
  device["name"]         = s_client_name;
}

static void publishDiscovery() {
  // Motion binary_sensor
  publishDiscoveryOne(d_motion, [](StaticJsonDocument<512>& d){
    d["name"]                 = s_client_name + " Motion";
    d["unique_id"]            = s_client_name + "_" + chipId() + "_motion";
    d["state_topic"]          = t_motion;
    d["device_class"]         = "motion";
    d["payload_on"]           = "ON";
    d["payload_off"]          = "OFF";
    d["availability_topic"]   = t_status;
    addDeviceBlock(d);
  });

  // Button binary_sensor
  publishDiscoveryOne(d_button, [](StaticJsonDocument<512>& d){
    d["name"]                 = s_client_name + " Button";
    d["unique_id"]            = s_client_name + "_" + chipId() + "_button";
    d["state_topic"]          = t_button;
    d["payload_on"]           = "ON";
    d["payload_off"]          = "OFF";
    d["availability_topic"]   = t_status;
    addDeviceBlock(d);
  });

  // Light sensor (ADC)
  publishDiscoveryOne(d_light, [](StaticJsonDocument<512>& d){
    d["name"]                 = s_client_name + " Light";
    d["unique_id"]            = s_client_name + "_" + chipId() + "_light";
    d["state_topic"]          = t_light;
    d["unit_of_measurement"]  = "adc";
    d["state_class"]          = "measurement";
    d["availability_topic"]   = t_status;
    addDeviceBlock(d);
  });

  // Temperature sensor
  publishDiscoveryOne(d_temp, [](StaticJsonDocument<512>& d){
    d["name"]                 = s_client_name + " Temperature";
    d["unique_id"]            = s_client_name + "_" + chipId() + "_temperature";
    d["state_topic"]          = t_temp;
    d["device_class"]         = "temperature";
    d["unit_of_measurement"]  = "°C";
    d["state_class"]          = "measurement";
    d["availability_topic"]   = t_status;
    addDeviceBlock(d);
  });

  // Guard switch
  publishDiscoveryOne(d_guard, [](StaticJsonDocument<512>& d){
    d["name"]                 = s_client_name + " Guard Mode";
    d["unique_id"]            = s_client_name + "_" + chipId() + "_guard";
    d["command_topic"]        = t_guard_set;
    d["state_topic"]          = t_guard_state;
    d["payload_on"]           = "ON";
    d["payload_off"]          = "OFF";
    d["availability_topic"]   = t_status;
    addDeviceBlock(d);
  });
}

// ------------------------ onMessage ------------------------
static void onMessage(char* topic, byte* payload, unsigned int length) {
  String t = topic;
  String msg; msg.reserve(length);
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];

  if (t == t_guard_set) {
    bool new_state = (msg == "ON");
    if (new_state != s_guard) {
      s_guard = new_state;
      if (s_guard_cb) s_guard_cb(s_guard);
    }
    s_mqtt->publish(t_guard_state.c_str(), s_guard ? "ON" : "OFF", true); // retain
  }
}

// ============================================================
//                       PUBLIC  API
// ============================================================
namespace mqtt_ha {

void begin(PubSubClient& mqtt, AppConfig& cfg, const String& client_name, GuardChangeCb cb) {
  s_mqtt = &mqtt;
  s_cfg = &cfg;
  s_client_name = client_name;
  s_guard_cb = cb;

  s_mqtt->setCallback(onMessage);
  s_mqtt->setBufferSize(1024);    // важно для discovery JSON
  ensureTopics();
}

// >>> меняем сигнатуру и поведение:
bool ensureMqtt(bool ap_mode) {
  if (ap_mode || WiFi.status() != WL_CONNECTED || !s_mqtt) return false;
  if (s_mqtt->connected()) return false;

  String cid = s_cfg->client_id.length() ? s_cfg->client_id : (s_client_name + "-" + chipId());
  s_mqtt->setServer(s_cfg->host.c_str(), s_cfg->port);

  if (s_mqtt->connect(
        cid.c_str(),
        s_cfg->user.length() ? s_cfg->user.c_str() : nullptr,
        s_cfg->pass.length() ? s_cfg->pass.c_str() : nullptr,
        t_status.c_str(), 0, true, "offline"   // LWT retained
      )) {

    s_mqtt->publish(t_status.c_str(), "online", true);  // retained

    publishDiscovery();                 // retained
    s_mqtt->subscribe(t_guard_set.c_str());
    s_mqtt->publish(t_guard_state.c_str(), s_guard ? "ON" : "OFF", true);  // retained

    // Помечаем, что началась новая сессия — чтобы публикации «по изменению» точно пошли
    s_connected_session = true;
    s_last_motion = 2;
    s_last_button = 2;
    s_last_light  = -32768;
    s_last_temp   = NAN;
    s_last_temp_pub_ms = 0;

    return true;   // <<< сообщаем main, что подключились прямо сейчас
  }
  return false;
}

// Публикации только при изменении, НО теперь с retain=true
void publishFast(bool motion, bool button, int light) {
  if (!s_mqtt || !s_mqtt->connected()) return;

  if (s_connected_session || s_last_motion == 2 || (int)motion != s_last_motion) {
    s_mqtt->publish(t_motion.c_str(), motion ? "ON" : "OFF", true);   // retained
    s_last_motion = (int)motion;
  }

  if (s_connected_session || s_last_button == 2 || (int)button != s_last_button) {
    s_mqtt->publish(t_button.c_str(), button ? "ON" : "OFF", true);   // retained
    s_last_button = (int)button;
  }

  if (s_connected_session || s_last_light == -32768 || abs(light - s_last_light) >= LIGHT_DELTA) {
    char buf[12]; snprintf(buf, sizeof(buf), "%d", light);
    s_mqtt->publish(t_light.c_str(), buf, true);                      // retained
    s_last_light = light;
  }

  s_connected_session = false;
}

void publishTemp(double tC) {
  if (!s_mqtt || !s_mqtt->connected()) return;

  const unsigned long now = millis();
  bool need = false;

  if (isnan(s_last_temp)) need = true;
  else if (fabs(tC - s_last_temp) >= TEMP_DELTA) need = true;
  else if (now - s_last_temp_pub_ms >= TEMP_MAX_AGE_MS) need = true;

  if (need) {
    char tb[16]; dtostrf(tC, 0, 2, tb);
    s_mqtt->publish(t_temp.c_str(), tb, true);                        // retained
    s_last_temp = tC;
    s_last_temp_pub_ms = now;
  }
}

} // namespace
