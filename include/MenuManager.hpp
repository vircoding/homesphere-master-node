#pragma once

#include <LiquidCrystal.h>

#include <functional>
#include <map>

#include "KeypadManager.hpp"
#include "NowManager.hpp"
#include "WebServerManager.hpp"

class MenuManager {
 public:
  enum class State {
    MAIN,
    STATUS,
    SENSOR,
    ACTUATOR,
    ACTUATOR_SET_CONFIRM,
    SCHEDULE_ACTUATOR_CONNECTION,
    SCHEDULE_ACTUATOR_DESCONNECTION,
    SCHEDULE_ACTUATOR_CONFIRM,
    CONFIG,
    CONFIG_CONFIRM,
    CONFIG_EXIT_CONFIRM,
    ABOUT,
    WELCOME,
  };

  enum class Event {
    CONFIG_ENTER,
    CONFIG_EXIT,
    SET_ACTUATOR,
    SCHEDULE_ACTUATOR,
  };

  struct Data {
    bool wifi;
    bool internet;
    String ipAP;
  };

  struct MenuItem {
    const char* text;
    State nextState;
  };

  struct ScheduleActuatorTimeItem {
    const char* text;
    const uint32_t time;
  };

  struct ActuatorSchedule {
    uint32_t offset = 0;
    uint32_t duration = 0xFFFFFFFF;
  };

  MenuManager(const gpio_num_t lcdRS, const gpio_num_t lcdEN,
              const gpio_num_t lcdD4, const gpio_num_t lcdD5,
              const gpio_num_t lcdD6, const gpio_num_t lcdD7,
              WebServerManager& server, Data& data, NowManager& now);
  void begin();
  void handleKey(Key key);
  void updateDisplay();
  void showCustomInfoScreen(const String& line1, const String& line2);
  void clearCustomInfoScreen();
  void tryConnect(const String& ssid);
  void on(Event event, std::function<void()> callback);
  void updateData();
  State getState() const { return _currentState; };
  int getCurrentActuatorIndex() const { return _actuatorScreen; };
  ActuatorSchedule getActuatorSchedule() const { return _actuatorSchedule; };

 private:
  LiquidCrystal _lcd;
  Data& _data;
  NowManager& _now;
  ActuatorSchedule _actuatorSchedule;

  // Variables de estado
  State _currentState = State::MAIN;
  uint8_t _menuPosition = 0;
  uint8_t _displayStart = 0;
  uint8_t _sensorScreen = 0;
  uint8_t _actuatorScreen = 0;
  uint8_t _actuatorOption = 0;
  uint8_t _scheduleActuatorConnectionOption = 0;
  uint8_t _scheduleActuatorConnectionDisplayStart = 0;
  uint8_t _scheduleActuatorDesconnectionOption = 0;
  uint8_t _scheduleActuatorDesconnectionDisplayStart = 0;
  uint8_t _confirmOption = 0;
  bool _stopKeypad = false;

  // Configuración del menu
  static constexpr uint8_t MAIN_MENU_COUNT = 5;
  static constexpr uint8_t SCHEDULE_ACTUATOR_CONNECTION_TIMES_COUNT = 8;
  static constexpr uint8_t SCHEDULE_ACTUATOR_DESCONNECTION_TIMES_COUNT = 8;

  const MenuItem _mainMenuItems[5] = {
      {"Sensores", State::SENSOR}, {"Actuadores", State::ACTUATOR},
      {"Estado", State::STATUS},   {"Configuracion", State::CONFIG_CONFIRM},
      {"Acerca de", State::ABOUT},
  };

  const ScheduleActuatorTimeItem _scheduleActuatorConnectionTimes[8] = {
      {"Ahora", 0},          {"15min", 1000 * 15}, {"30min", 1000 * 30},
      {"45min", 1000 * 45},  {"1h", 1000 * 60},    {"2h", 1000 * 60 * 2},
      {"4h", 1000 * 60 * 4}, {"8h", 1000 * 60 * 8}};

  const ScheduleActuatorTimeItem _scheduleActuatorDesconnectionTimes[8] = {
      {"Nunca", 0xFFFFFFFF}, {"5min", 1000 * 5},   {"15min", 1000 * 15},
      {"30min", 1000 * 30},  {"45min", 1000 * 45}, {"1h", 1000 * 60},
      {"2h", 1000 * 60 * 2}, {"4h", 1000 * 60 * 4}};

  // const ScheduleActuatorTimeItem _scheduleActuatorConnectionTimes[8] = {
  //     {"Ahora", 0},
  //     {"15min", 1000 * 60 * 15},
  //     {"30min", 1000 * 60 * 30},
  //     {"45min", 1000 * 60 * 45},
  //     {"1h", 1000 * 60 * 60},
  //     {"2h", 1000 * 60 * 60 * 2},
  //     {"4h", 1000 * 60 * 60 * 4},
  //     {"8h", 1000 * 60 * 60 * 8}};

  // const ScheduleActuatorTimeItem _scheduleActuatorDesconnectionTimes[8] = {
  //     {"Nunca", 0xFFFFFFFF},      {"5min", 1000 * 60 * 5},
  //     {"15min", 1000 * 60 * 15},  {"30min", 1000 * 60 * 30},
  //     {"45min", 1000 * 60 * 45},  {"1h", 1000 * 60 * 60},
  //     {"2h", 1000 * 60 * 60 * 2}, {"4h", 1000 * 60 * 60 * 4}};

  // Callbacks de eventos
  std::map<Event, std::function<void()>> _callbacks;

  // Métodos privados
  void _trigger(Event event);
  void _showMain();
  void _showStatus();
  void _showSensor();
  void _showActuator();
  void _showScheduleActuatorConnection();
  void _showScheduleActuatorDesconnection();
  void _showConfig();
  void _showAbout();
  void _showConfirm(const String& message);
};