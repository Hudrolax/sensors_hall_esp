# README.md

# ESP8266 Sensor Outside (PlatformIO + MQTT + HA Discovery + Web OTA)

Проект для **ESP8266 (NodeMCU v2)** с OLED (SH1106), датчиком температуры **DS18B20** (OneWire), датчиком движения **HC-SR501**, аналоговым датчиком освещённости (A0), кнопкой и зуммером.  
Функциональность:
- Переписано под **PlatformIO** (Arduino framework).
- **MQTT-клиент** с поддержкой **Home Assistant MQTT Discovery**:
  - `binary_sensor` — движение (PIR)
  - `sensor` — освещённость (ADC 0..1023)
  - `sensor` — температура (°C) с DS18B20
  - `binary_sensor` — кнопка
  - `switch` — `guard_mode` (переключаемый из HA; при смене играет мелодия)
  - `availability` — `online/offline`
- **NTP-время**: берётся из интернета, выводится на OLED в **GMT+3**.
- **Web-интерфейс** (без авторизации):
  - страница настроек MQTT: сервер/порт/логин/пароль/ClientID
  - перезагрузка устройства
- **Web OTA**: загрузка бинарника прошивки по адресу `/update` (без пароля).
- **Пины оставлены как в исходном проекте**.
- Зуммер и мелодии — **без изменений**.

---

## Аппаратные пины (как в исходнике)

| Узел                     | Пин MCU | Примечание                                  |
|-------------------------|---------|---------------------------------------------|
| OneWire (DS18B20)       | D2 / GPIO4 | `#define ONE_WIRE_BUS 4`                  |
| PIR HC-SR501            | D1 / GPIO5 | `#define hc_sr501_pin 5`                  |
| Аналог. освещённость    | A0      | На NodeMCU уже есть делитель до ~3.3 В      |
| Кнопка                  | D5 / GPIO14 | `#define button_pin 14`                  |
| Зуммер                  | D8 / GPIO15 | `#define buzzerPin 15`                   |
| OLED (SH1106, I²C)      | SDA=GPIO0 (D3), SCL=GPIO2 (D4) | `Wire.begin(0, 2)` как в исходнике |

> **Важно:** GPIO0 и GPIO2 — загрузочные пины ESP8266. Вы их просили **не менять**, но учитывайте это при реальной разводке.

---

## Зависимости

Они подтягиваются автоматически по `platformio.ini`:

- OneWire
- DallasTemperature
- U8g2 (SH1106)
- PubSubClient (MQTT)
- ArduinoJson
- LittleFS (файловая система, встроена в платформу)

---

Конфигурация MQTT сохраняется во Flash (LittleFS) в файле: **`/config.json`** (создаётся после сохранения настроек на веб-странице).

---

## Сборка и прошивка

Требуется установленный **PlatformIO** (в виде расширения VS Code или CLI).

```bash
# Сборка
pio run

# Заливка прошивки по USB
pio run -t upload

# Монитор порта
pio device monitor -b 115200
```

Полная очистка Flash (если нужно «забыть» настройки в LittleFS):
```bash
pio run -t erase
```

Первичный запуск
	1.	Подключите плату по USB и прошейте проект.
	2.	Устройство подключится к Wi-Fi, заданному в main.cpp (макросы STASSID / STAPSK).
	3.	Определите IP устройства (в мониторинге порта или по списку клиентов роутера).
	4.	Откройте в браузере: http://<IP_ESP>/ — страница настроек MQTT.
	5.	Заполните MQTT-параметры и нажмите Save & Reboot.

⸻

Веб-интерфейс (без авторизации)
	•	/ — главная страница настроек MQTT (Host, Port, Username, Password, Client ID), кнопки:
	•	Save & Reboot — сохранить конфиг и перезагрузить.
	•	Firmware Update — перейти на страницу OTA.
	•	Reboot — просто перезагрузить устройство.
	•	/update — OTA-прошивка: загрузите готовый *.bin.
(Страница без пароля — как вы и просили.)

⸻

MQTT и Home Assistant

Базовые топики

По умолчанию (см. client_name = "sensor_outside"):
```
esp/sensor_outside/status
esp/sensor_outside/motion/state
esp/sensor_outside/button/state
esp/sensor_outside/light/state
esp/sensor_outside/temperature/state
esp/sensor_outside/guard/state
esp/sensor_outside/guard/set
```
	•	status: LWT/availability (online/offline)
	•	motion/state: ON/OFF
	•	button/state: ON/OFF
	•	light/state: целое 0..1023 (ADC)
	•	temperature/state: число (°C)
	•	guard/state: ON/OFF (отражение текущего состояния)
	•	guard/set: принимается ON/OFF (управление из HA, проигрываются мелодии)

