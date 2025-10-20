#pragma once
#include <PubSubClient.h>
#include "config.h"

namespace mqtt_ha {

  using GuardChangeCb = void(*)(bool new_state);

  void begin(PubSubClient& mqtt, AppConfig& cfg, const String& client_name, GuardChangeCb cb);

  // было: void ensureMqtt(bool ap_mode);
  // стало: вернуть true, если в этом цикле произошло подключение
  bool ensureMqtt(bool ap_mode);

  void publishFast(bool motion, bool button, int light);
  void publishTemp(double tC);

}
