#include <LittleFS.h>
#include <esp_task_wdt.h>
#include <freertos/FreeRTOS.h>

#include "ConfigManager.hpp"
#include "IndicatorManager.hpp"
#include "KeypadManager.hpp"
#include "MenuManager.hpp"
#include "NowManager.hpp"
#include "SyncButtonManager.hpp"
#include "Utils.hpp"
#include "WebServerManager.hpp"
#include "WiFiManager.hpp"

// Pinout
const uint8_t rgbRed = 4;
const uint8_t rgbGreen = 16;
const uint8_t rgbBlue = 17;
const uint8_t keypadUp = 23;
const uint8_t keypadDown = 22;
const uint8_t keypadBack = 21;
const uint8_t keypadEnter = 19;
const uint8_t syncButtonPin = 18;
const uint8_t lcdRS = 13;
const uint8_t lcdEN = 14;
const uint8_t lcdD4 = 27;
const uint8_t lcdD5 = 26;
const uint8_t lcdD6 = 25;
const uint8_t lcdD7 = 33;

// Task Handlers
TaskHandle_t blinkRGBTaskHandler = NULL;
TaskHandle_t sendSyncBroadcastTaskHandler = NULL;

// Timer Handlers
TimerHandle_t syncModeTimeoutTimerHandler = NULL;

// Global Variables
MenuManager::Data globalData;
uint32_t wdtTimeout = 5;

ConfigManager config;
WiFiManager wifi;
WebServerManager server(config, wifi);
IndicatorManager rgb(rgbRed, rgbGreen, rgbBlue);
KeypadManager keypad(keypadUp, keypadDown, keypadBack, keypadEnter);
SyncButtonManager syncButton(syncButtonPin);
MenuManager menu(lcdRS, lcdEN, lcdD4, lcdD5, lcdD6, lcdD7, server, globalData);
NowManager now;

// Definitions
void setWatchdogTimeout(uint32_t newTimeout);
void tryConnectSTA(const bool updateDisplay = false);
void onConfigEnterCallback();
void onConfigExitCallback();
void onReceivedCallback(const uint8_t* mac_addr, const uint8_t* data,
                        int length);
void handleMenuTask(void* parameter);
void updateMenuTask(void* parameter);
void blinkRGBTask(void* parameter);
void sendSyncBroadcastTask(void* parameter);
void enterSyncMode();
void endSyncMode();
void onLongButtonPressCallback() { enterSyncMode(); };
void onSimpleButtonPressCallback() { endSyncMode(); };
void onRegistrationReceivedCallback(const uint8_t* mac, const uint8_t* data,
                                    int length);
void syncModeTimeoutCallback(TimerHandle_t xTimer) { endSyncMode(); };
void registerAllNodes(const uint8_t size);

void setup() {
  Serial.begin(115200);

  if (!config.init()) {
    ESP.restart();
  }

  wifi.modeAPSTA();

  menu.on(MenuManager::Event::CONFIG_ENTER, onConfigEnterCallback);
  menu.on(MenuManager::Event::CONFIG_EXIT, onConfigExitCallback);

  menu.begin();
  menu.showWelcome();

  rgb.begin();
  keypad.begin();

  syncButton.begin();
  syncButton.on(SyncButtonManager::Event::SIMPLE_PRESS,
                onSimpleButtonPressCallback);
  syncButton.on(SyncButtonManager::Event::LONG_PRESS,
                onLongButtonPressCallback);

  now.init();

  Serial.printf("Cantidad de nodos en /config.json: %d\n",
                config.getNodeLength());
  registerAllNodes(config.getNodeLength());

  // now.onReceived(onReceivedCallback);

  menu.updateDisplay();

  // Tasks
  xTaskCreatePinnedToCore(handleMenuTask, "Handle Menu", 10000, NULL, 1, NULL,
                          0);
  xTaskCreatePinnedToCore(updateMenuTask, "Update Menu", 4096, NULL, 1, NULL,
                          0);
}

void loop() { syncButton.update(); }

void setWatchdogTimeout(uint32_t newTimeout) {
  wdtTimeout = newTimeout;
  esp_task_wdt_init(wdtTimeout, false);
}

void tryConnectSTA(const bool updateDisplay) {
  const ConfigManager::NetworkConfig staConfig = config.getSTAConfig();

  rgb.set(Status::PENDING);

  if (updateDisplay) {
    menu.tryConnect(staConfig.ssid);
  }

  if (wifi.connectSTA(staConfig.ssid, staConfig.password)) {
    globalData.wifi = true;
    rgb.set(Status::ONLINE);
  } else {
    globalData.wifi = false;
    rgb.set(Status::OFFLINE);
  }

  if (updateDisplay) {
    menu.updateDisplay();
  }
}

void onConfigEnterCallback() {
  server.setupRoutes();
  globalData.ipAP = server.begin();
}

void onConfigExitCallback() { server.end(); }

void onReceivedCallback(const uint8_t* mac_addr, const uint8_t* data,
                        int length) {
  NowManager::TemperatureData* recvData = (NowManager::TemperatureData*)data;
  Serial.print("Dato recibido de nodo: ");
  Serial.println(recvData->node_id);

  Serial.printf("Temperatura: %fC\n", recvData->temp);
  Serial.printf("Humedad: %f%\n", recvData->hum);

  globalData.temp = recvData->temp;
  globalData.hum = recvData->hum;
}

void handleMenuTask(void* parameter) {
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

void updateMenuTask(void* parameter) {
  while (1) {
    menu.updateData();

    // Delay - 3s
    vTaskDelay(pdMS_TO_TICKS(3000));
  }
}

void blinkRGBTask(void* parameter) {
  while (1) {
    rgb.set(Status::PENDING);
    vTaskDelay(pdMS_TO_TICKS(500));  // 500ms

    rgb.set(Status::OFF);
    vTaskDelay(pdMS_TO_TICKS(500));  // 500ms
  }
}

void sendSyncBroadcastTask(void* parameter) {
  while (1) {
    if (now.sendSyncBroadcastMsg()) {
      Serial.println("Mensaje SyncBroadcast enviado");
    } else
      Serial.println("Error enviando el mensaje Syncroadcast");

    vTaskDelay(
        pdMS_TO_TICKS(NowManager::SEND_SYNC_BROADCAST_MSG_INTERVAL));  // 10s
  }
}

void enterSyncMode() {
  if (blinkRGBTaskHandler == NULL && sendSyncBroadcastTaskHandler == NULL) {
    if (!now.reset()) ESP.restart();

    if (!now.registerBroadcastPeer()) return;

    now.onReceived(onRegistrationReceivedCallback);

    xTaskCreatePinnedToCore(blinkRGBTask, "Blink LED", 2048, NULL, 2,
                            &blinkRGBTaskHandler, 1);
    xTaskCreatePinnedToCore(sendSyncBroadcastTask, "Send Sync Broadcast", 4096,
                            NULL, 1, &sendSyncBroadcastTaskHandler, 1);

    syncModeTimeoutTimerHandler = xTimerCreate(
        "Sync Mode Timeout", pdMS_TO_TICKS(NowManager::SYNC_MODE_TIMEOUT),
        pdFALSE, (void*)0,
        syncModeTimeoutCallback);  // 30s

    if (syncModeTimeoutTimerHandler != NULL)
      xTimerStart(syncModeTimeoutTimerHandler, 0);
  }
}

void onRegistrationReceivedCallback(const uint8_t* mac, const uint8_t* data,
                                    int length) {
  if (NowManager::validateMessage(NowManager::MessageType::REGISTRATION, data,
                                  length)) {
    const NowManager::RegistrationMsg* msg =
        reinterpret_cast<const NowManager::RegistrationMsg*>(data);

    if (verifyCRC8(*msg) &&
        config.saveNodeConfig(mac, msg->nodeType, msg->firmwareVersion) &&
        now.addDevice(mac, msg->nodeType, msg->firmwareVersion)) {
      if (now.sendConfirmRegistrationMsg(mac)) {
        Serial.println("Mensaje ConfirmRegistration enviado");
      } else {
        Serial.println("Error enviando el mensaje ConfirmRegistration");
      }
    }

    endSyncMode();
  }
}

void registerAllNodes(const uint8_t size) {
  for (uint8_t i = 0; i < size; i++) {
    ConfigManager::NodeInfo node = config.getNode(i);
    now.addDevice(node.mac, node.nodeType, node.firmwareVersion);
  }

  now.printAllDevices();
}

void endSyncMode() {
  // Detener el timer
  xTimerStop(syncModeTimeoutTimerHandler, 0);

  // Eliminar el timer y liberar memoria
  if (xTimerDelete(syncModeTimeoutTimerHandler, 0) == pdPASS)
    syncModeTimeoutTimerHandler =
        NULL;  // Reasignar a NULL para evitar usos indebidos

  if (sendSyncBroadcastTaskHandler != NULL) {
    vTaskDelete(sendSyncBroadcastTaskHandler);
    sendSyncBroadcastTaskHandler = NULL;
  }

  if (blinkRGBTaskHandler != NULL) {
    vTaskDelete(blinkRGBTaskHandler);
    blinkRGBTaskHandler = NULL;
    rgb.set(Status::OFF);
  }

  // Reiniciar ESP-NOW
  if (!now.reset()) ESP.restart();
}
