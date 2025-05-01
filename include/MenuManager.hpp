#pragma once

#include <LiquidCrystal.h>

#include <functional>
#include <map>

#include "KeypadManager.hpp"
#include "WebServerManager.hpp"

class MenuManager {
 public:
  enum class State {
    MAIN,
    STATUS,
    SENSOR,
    ACTION,
    CONFIG,
    CONFIG_CONFIRM,
    CONFIG_EXIT_CONFIRM,
    ABOUT,
    WELCOME,
  };

  enum class Event {
    CONFIG_ENTER,
    CONFIG_EXIT,
  };

  struct Data {
    bool wifi;
    bool internet;
    String ipAP;
    float temp;
    float hum;
  };

  struct MenuItem {
    const char* text;
    State nextState;
  };

  MenuManager(uint8_t lcdRS, uint8_t lcdEN, uint8_t lcdD4, uint8_t lcdD5,
              uint8_t lcdD6, uint8_t lcdD7, WebServerManager& server,
              Data& data);
  void begin();
  void handleKey(Key key);
  void updateDisplay();
  void showWelcome();
  void tryConnect(const String& ssid);
  void on(Event event, std::function<void()> callback);
  void updateData();
  State getState() const { return _currentState; };

 private:
  LiquidCrystal _lcd;
  Data& _data;

  // Variables de estado
  State _currentState = State::MAIN;
  uint8_t _menuPosition = 0;
  uint8_t _displayStart = 0;
  uint8_t _sensorScreen = 0;
  uint8_t _actuatorScreen = 0;
  uint8_t _confirmOption = 0;

  // Configuración del menu
  static constexpr uint8_t MAIN_MENU_COUNT = 5;

  const MenuItem _mainMenuItems[5] = {
      {"Estado", State::STATUS},     {"Sensores", State::SENSOR},
      {"Actuadores", State::ACTION}, {"Configuracion", State::CONFIG_CONFIRM},
      {"Acerca de", State::ABOUT},
  };

  // Callbacks de eventos
  std::map<Event, std::function<void()>> _callbacks;

  // Métodos privados
  void _trigger(Event event);
  void _showMain();
  void _showStatus();
  void _showSensor();
  void _showAction();
  void _showConfig();
  void _showAbout();
  void _showConfirm(const String& message);
};