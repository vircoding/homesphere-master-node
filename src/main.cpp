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
const gpio_num_t rgbRed = GPIO_NUM_4;
const gpio_num_t rgbGreen = GPIO_NUM_16;
const gpio_num_t rgbBlue = GPIO_NUM_17;
const gpio_num_t keypadUp = GPIO_NUM_23;
const gpio_num_t keypadDown = GPIO_NUM_22;
const gpio_num_t keypadBack = GPIO_NUM_21;
const gpio_num_t keypadEnter = GPIO_NUM_19;
const gpio_num_t syncButtonPin = GPIO_NUM_18;
const gpio_num_t lcdRS = GPIO_NUM_13;
const gpio_num_t lcdEN = GPIO_NUM_14;
const gpio_num_t lcdD4 = GPIO_NUM_27;
const gpio_num_t lcdD5 = GPIO_NUM_26;
const gpio_num_t lcdD6 = GPIO_NUM_25;
const gpio_num_t lcdD7 = GPIO_NUM_33;

// Task Handlers
TaskHandle_t blinkRGBTaskHandler = NULL;
TaskHandle_t sendSyncBroadcastTaskHandler = NULL;

// Timer Handlers
TimerHandle_t syncModeTimeoutTimerHandler = NULL;

// Global Variables
bool syncModeState = false;
MenuManager::Data globalData;
uint32_t wdtTimeout = 5;

ConfigManager config;
WiFiManager wifi;
WebServerManager server(config, wifi);
IndicatorManager rgb(rgbRed, rgbGreen, rgbBlue);
KeypadManager keypad(keypadUp, keypadDown, keypadBack, keypadEnter);
SyncButtonManager syncButton(syncButtonPin);
NowManager now;
MenuManager menu(lcdRS, lcdEN, lcdD4, lcdD5, lcdD6, lcdD7, server, globalData,
                 now);

// Definitions
void setWatchdogTimeout(uint32_t newTimeout);
void tryConnectSTA(const bool updateDisplay = false);
void onConfigEnterCallback();
void onConfigExitCallback();
void onReceivedCallback(const uint8_t* mac, const uint8_t* data, int length);
void handleMenuTask(void* parameter);
void blinkRGBTask(void* parameter);
void sendSyncBroadcastTask(void* parameter);
void enterSyncMode();
void endSyncMode();
void onLongButtonPressCallback() { enterSyncMode(); };
void onSimpleButtonPressCallback() {
  const uint8_t mac[6] = {0x08, 0xA6, 0xF7, 0xBC, 0x8C, 0xA8};

  if (now.sendScheduleActionMsg(mac, 8000, 15000))
    Serial.println("Mensaje ScheduleAction enviado");
  else
    Serial.println("Error enviando mensaje ScheduleAction");

  endSyncMode();
};
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
  menu.showCustomInfoScreen("HomeSphere", "Bienvenid@");

  rgb.begin();
  keypad.begin();

  syncButton.begin();
  syncButton.on(SyncButtonManager::Event::SIMPLE_PRESS,
                onSimpleButtonPressCallback);
  syncButton.on(SyncButtonManager::Event::LONG_PRESS,
                onLongButtonPressCallback);

  now.init();
  registerAllNodes(config.getNodeLength());
  now.onReceived(onReceivedCallback);

  menu.clearCustomInfoScreen();

  // Tasks
  xTaskCreatePinnedToCore(handleMenuTask, "Handle Menu", 10000, NULL, 1, NULL,
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

void onReceivedCallback(const uint8_t* mac, const uint8_t* data, int length) {
  // Validar que la direccion mac este en la lista de paired devices
  if (!now.isDevicePaired(mac)) return;

  if (NowManager::validateMessage(NowManager::MessageType::TEMPERATURE_HUMIDITY,
                                  data, length)) {
    const NowManager::TemperatureHumidityMsg* msg =
        reinterpret_cast<const NowManager::TemperatureHumidityMsg*>(data);

    if (verifyCRC8(*msg)) {
      now.updateSensorData(mac, "Temp", msg->temp);
      now.updateSensorData(mac, "Hum", msg->hum);

      now.updateDeviceLastSeen(mac);
      menu.updateData();
    }
  }
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
    now.sendSyncBroadcastMsg();

    vTaskDelay(
        pdMS_TO_TICKS(NowManager::SEND_SYNC_BROADCAST_MSG_INTERVAL));  // 10s
  }
}

void enterSyncMode() {
  if (!syncModeState) {
    menu.showCustomInfoScreen("Vinculando", "nodo secundario");

    if (!now.reset()) ESP.restart();

    if (!now.registerBroadcastPeer()) return;

    now.onReceived(onRegistrationReceivedCallback);

    if (blinkRGBTaskHandler == NULL) {
      xTaskCreatePinnedToCore(blinkRGBTask, "Blink LED", 2048, NULL, 2,
                              &blinkRGBTaskHandler, 1);
    }

    if (sendSyncBroadcastTaskHandler == NULL) {
      xTaskCreatePinnedToCore(sendSyncBroadcastTask, "Send Sync Broadcast",
                              4096, NULL, 1, &sendSyncBroadcastTaskHandler, 1);
    }

    syncModeTimeoutTimerHandler = xTimerCreate(
        "Sync Mode Timeout", pdMS_TO_TICKS(NowManager::SYNC_MODE_TIMEOUT),
        pdFALSE, (void*)0,
        syncModeTimeoutCallback);  // 30s

    if (syncModeTimeoutTimerHandler != NULL)
      xTimerStart(syncModeTimeoutTimerHandler, 0);

    syncModeState = true;
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
        now.addDevice(mac, msg->nodeType, "Nodo Secundario",
                      msg->firmwareVersion))
      now.sendConfirmRegistrationMsg(mac);

    endSyncMode();
  }
}

void registerAllNodes(const uint8_t size) {
  for (uint8_t i = 0; i < size; i++) {
    ConfigManager::NodeInfo node = config.getNode(i);
    now.addDevice(node.mac, node.nodeType, node.deviceName,
                  node.firmwareVersion);
  }

  now.printAllDevices();
}

void endSyncMode() {
  if (syncModeState) {
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

    registerAllNodes(config.getNodeLength());
    now.onReceived(onReceivedCallback);
    menu.clearCustomInfoScreen();

    syncModeState = false;
  }
}
