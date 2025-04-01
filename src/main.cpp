#include <esp_task_wdt.h>
#include <freertos/FreeRTOS.h>

#include "ConfigManager.hpp"
#include "IndicatorManager.hpp"
#include "KeypadManager.hpp"
#include "MenuManager.hpp"
#include "WebServerManager.hpp"
#include "WiFiManager.hpp"

const uint8_t rgbRed = 16;
const uint8_t rgbGreen = 17;
const uint8_t rgbBlue = 18;
const uint8_t keypadUp = 23;
const uint8_t keypadDown = 22;
const uint8_t keypadBack = 21;
const uint8_t keypadEnter = 19;
const uint8_t lcdRS = 13;
const uint8_t lcdEN = 14;
const uint8_t lcdD4 = 27;
const uint8_t lcdD5 = 26;
const uint8_t lcdD6 = 25;
const uint8_t lcdD7 = 33;

MenuManager::Data data;
uint32_t wdtTimeout = 5;  // Valor inicial (segundos)

ConfigManager config;
WiFiManager wifi;
WebServerManager server(config, wifi);
IndicatorManager rgb(rgbRed, rgbGreen, rgbBlue);
KeypadManager keypad(keypadUp, keypadDown, keypadBack, keypadEnter);
MenuManager menu(lcdRS, lcdEN, lcdD4, lcdD5, lcdD6, lcdD7, server, data);

void setWatchdogTimeout(uint32_t new_timeout) {
  wdtTimeout = new_timeout;
  esp_task_wdt_init(wdtTimeout, false);
}

void tryConnectSTA(const bool updateDisplay = false) {
  const ConfigManager::NetworkConfig staConfig = config.getSTAConfig();

  rgb.setIndicator(Status::PENDING);

  if (updateDisplay) {
    menu.tryConnect(staConfig.ssid);
  }

  if (wifi.connectSTA(staConfig.ssid, staConfig.password)) {
    data.wifi = true;
    rgb.setIndicator(Status::ONLINE);
  } else {
    data.wifi = false;
    rgb.setIndicator(Status::OFFLINE);
  }

  if (updateDisplay) {
    menu.updateDisplay();
  }
}

void onConfigEnter() {
  server.setupRoutes();
  data.ipAP = server.begin();
}

void onConfigExit() { server.end(); }

void taskHandleMenu(void* parameter) {
  bool isSuscribed = false;

  while (1) {
    Key key = keypad.getKey();

    if (key != Key::NONE) {
      menu.handleKey(key);
      menu.updateDisplay();
    }

    if (isSuscribed) {
      esp_task_wdt_reset();  // Resetear el watchdog

      if (!server.getIsListening()) {
        setWatchdogTimeout(5);
        esp_task_wdt_delete(NULL);  // Eliminar suscripción al watchdog
        isSuscribed = false;
      }
    } else {
      if (server.getIsListening()) {
        setWatchdogTimeout(10);
        esp_task_wdt_add(NULL);  // Suscribir tarea al watchdog
        isSuscribed = true;
      }
    }

    // Delay - 10ms
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void setup() {
  Serial.begin(115200);

  if (!config.init()) {
    ESP.restart();
  }

  menu.on(MenuManager::Event::CONFIG_ENTER, onConfigEnter);
  menu.on(MenuManager::Event::CONFIG_EXIT, onConfigExit);

  menu.begin();
  menu.showWelcome();

  rgb.begin();
  keypad.begin();

  tryConnectSTA();

  menu.updateDisplay();

  // Tasks
  xTaskCreatePinnedToCore(taskHandleMenu, "Handle Menu", 10000, NULL, 1, NULL,
                          0);
}

void loop() {}