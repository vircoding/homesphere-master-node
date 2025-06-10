#include "MenuManager.hpp"

#include <cmath>

#include "KeypadManager.hpp"
#include "Utils.hpp"

MenuManager::MenuManager(const gpio_num_t lcdRS, const gpio_num_t lcdEN,
                         const gpio_num_t lcdD4, const gpio_num_t lcdD5,
                         const gpio_num_t lcdD6, const gpio_num_t lcdD7,
                         WebServerManager& server, Data& data, NowManager& now)
    : _lcd(lcdRS, lcdEN, lcdD4, lcdD5, lcdD6, lcdD7), _data(data), _now(now) {}

void MenuManager::begin() { _lcd.begin(16, 2); }

void MenuManager::handleKey(Key key) {
  if (_stopKeypad) return;

  switch (_currentState) {
    case State::MAIN:
      if (key == Key::UP && _menuPosition > 0) {
        _menuPosition--;
        if (_menuPosition < _displayStart) _displayStart--;
      } else if (key == Key::DOWN && _menuPosition < MAIN_MENU_COUNT - 1) {
        _menuPosition++;
        if (_menuPosition - _displayStart >= 2) _displayStart++;
      } else if (key == Key::ENTER) {
        _currentState = _mainMenuItems[_menuPosition].nextState;
      }
      break;

    case State::CONFIG_CONFIRM:
      if (key == Key::UP) {
        _confirmOption = (_confirmOption > 0) ? _confirmOption - 1 : 0;
      } else if (key == Key::DOWN) {
        _confirmOption = (_confirmOption < 1) ? _confirmOption + 1 : 1;
      } else if (key == Key::ENTER) {
        if (_confirmOption == 1) {
          _currentState = State::CONFIG;

          _trigger(Event::CONFIG_ENTER);
        } else {
          _currentState = State::MAIN;
        }

        _confirmOption = 0;
      } else if (key == Key::BACK) {
        _currentState = State::MAIN;
        _confirmOption = 0;
      }
      break;

    case State::CONFIG_EXIT_CONFIRM:
      if (key == Key::UP) {
        _confirmOption = (_confirmOption > 0) ? _confirmOption - 1 : 0;
      } else if (key == Key::DOWN) {
        _confirmOption = (_confirmOption < 1) ? _confirmOption + 1 : 1;
      } else if (key == Key::ENTER) {
        if (_confirmOption == 1) {
          _currentState = State::MAIN;

          _trigger(Event::CONFIG_EXIT);
        } else {
          _currentState = State::CONFIG;
        }
        _confirmOption = 0;
      } else if (key == Key::BACK) {
        _currentState = State::CONFIG;
        _confirmOption = 0;
      }
      break;

    case State::CONFIG:
      if (key == Key::BACK) {
        _currentState = State::CONFIG_EXIT_CONFIRM;
      }
      break;

    case State::STATUS:
    case State::ABOUT:
      if (key == Key::BACK) _currentState = State::MAIN;
      break;

    case State::SENSOR:
      if (key == Key::UP || key == Key::DOWN) {
        const size_t size = _now.getSensorListSize();

        if (size > 0) {
          _sensorScreen = (_sensorScreen + 1) % size;
        }
      } else if (key == Key::BACK) {
        _currentState = State::MAIN;
        _sensorScreen = 0;
      }
      break;

    case State::ACTUATOR:
      if (key == Key::UP) {
        if (_actuatorOption > 0)
          _actuatorOption--;
        else {
          const size_t size = _now.getActuatorListSize();

          if (size > 0) {
            _actuatorScreen = (_actuatorScreen - 1) % size;
            _actuatorOption = 1;
          }
        }
      } else if (key == Key::DOWN) {
        if (_actuatorOption < 1)
          _actuatorOption++;
        else {
          const size_t size = _now.getActuatorListSize();

          if (size > 0) {
            _actuatorScreen = (_actuatorScreen + 1) % size;
            _actuatorOption = 0;
          }
        }
      } else if (key == Key::ENTER) {
        if (_actuatorOption == 1) {
          _currentState = State::SCHEDULE_ACTUATOR_CONNECTION;
          _actuatorOption = 0;
        } else if (_actuatorOption == 0) {
          _currentState = State::ACTUATOR_SET_CONFIRM;
        }
      } else if (key == Key::BACK) {
        _currentState = State::MAIN;
        _actuatorScreen = 0;
        _actuatorOption = 0;
      }

      break;

    case State::ACTUATOR_SET_CONFIRM:
      if (key == Key::UP) {
        _confirmOption = (_confirmOption > 0) ? _confirmOption - 1 : 0;
      } else if (key == Key::DOWN) {
        _confirmOption = (_confirmOption < 1) ? _confirmOption + 1 : 1;
      } else if (key == Key::ENTER) {
        if (_confirmOption == 1) {
          _currentState = State::ACTUATOR;

          _trigger(Event::SET_ACTUATOR);
        } else {
          _currentState = State::ACTUATOR;
        }
        _confirmOption = 0;
      } else if (key == Key::BACK) {
        _currentState = State::ACTUATOR;
        _confirmOption = 0;
      }

      break;

    case State::SCHEDULE_ACTUATOR_CONNECTION:
      if (key == Key::UP) {
        if (_scheduleActuatorConnectionOption == 0 &&
            _scheduleActuatorConnectionDisplayStart > 0)
          _scheduleActuatorConnectionDisplayStart--;

        if (_scheduleActuatorConnectionOption > 0)
          _scheduleActuatorConnectionOption--;
      } else if (key == Key::DOWN) {
        if (_scheduleActuatorConnectionOption == 3 &&
            _scheduleActuatorConnectionDisplayStart <
                SCHEDULE_ACTUATOR_CONNECTION_TIMES_COUNT - 4)
          _scheduleActuatorConnectionDisplayStart++;

        if (_scheduleActuatorConnectionOption < 3)
          _scheduleActuatorConnectionOption++;
      } else if (key == Key::ENTER) {
        _actuatorSchedule.offset =
            _scheduleActuatorConnectionTimes
                [_scheduleActuatorConnectionDisplayStart +
                 _scheduleActuatorConnectionOption]
                    .time;

        _currentState = State::SCHEDULE_ACTUATOR_DESCONNECTION;
        _scheduleActuatorConnectionOption = 0;
        _scheduleActuatorConnectionDisplayStart = 0;
      } else if (key == Key::BACK) {
        _currentState = State::ACTUATOR;
        _scheduleActuatorConnectionOption = 0;
        _scheduleActuatorConnectionDisplayStart = 0;
      }

      break;

    case State::SCHEDULE_ACTUATOR_DESCONNECTION:
      if (key == Key::UP) {
        if (_scheduleActuatorDesconnectionOption == 0 &&
            _scheduleActuatorDesconnectionDisplayStart > 0)
          _scheduleActuatorDesconnectionDisplayStart--;

        if (_scheduleActuatorDesconnectionOption > 0)
          _scheduleActuatorDesconnectionOption--;
      } else if (key == Key::DOWN) {
        if (_scheduleActuatorDesconnectionOption == 3 &&
            _scheduleActuatorDesconnectionDisplayStart <
                SCHEDULE_ACTUATOR_DESCONNECTION_TIMES_COUNT - 4)
          _scheduleActuatorDesconnectionDisplayStart++;

        if (_scheduleActuatorDesconnectionOption < 3)
          _scheduleActuatorDesconnectionOption++;
      } else if (key == Key::ENTER) {
        _actuatorSchedule.duration =
            _scheduleActuatorDesconnectionTimes
                [_scheduleActuatorDesconnectionDisplayStart +
                 _scheduleActuatorDesconnectionOption]
                    .time;

        _currentState = State::SCHEDULE_ACTUATOR_CONFIRM;
        _scheduleActuatorDesconnectionOption = 0;
        _scheduleActuatorDesconnectionDisplayStart = 0;
      } else if (key == Key::BACK) {
        _currentState = State::SCHEDULE_ACTUATOR_CONNECTION;
        _scheduleActuatorDesconnectionOption = 0;
        _scheduleActuatorDesconnectionDisplayStart = 0;
      }

      break;

    case State::SCHEDULE_ACTUATOR_CONFIRM:
      if (key == Key::UP) {
        _confirmOption = (_confirmOption > 0) ? _confirmOption - 1 : 0;
      } else if (key == Key::DOWN) {
        _confirmOption = (_confirmOption < 1) ? _confirmOption + 1 : 1;
      } else if (key == Key::ENTER) {
        if (_confirmOption == 1) {
          _currentState = State::ACTUATOR;

          _trigger(Event::SCHEDULE_ACTUATOR);
        } else {
          _currentState = State::ACTUATOR;
        }
        _confirmOption = 0;
      } else if (key == Key::BACK) {
        _currentState = State::SCHEDULE_ACTUATOR_DESCONNECTION;
        _confirmOption = 0;
      }

      break;
  }
};

void MenuManager::updateDisplay() {
  _lcd.clear();
  switch (_currentState) {
    case State::MAIN:
      _showMain();
      break;
    case State::STATUS:
      _showStatus();
      break;
    case State::SENSOR:
      _showSensor();
      break;
    case State::ACTUATOR:
      _showActuator();
      break;
    case State::ACTUATOR_SET_CONFIRM:
      _showConfirm(_now.getActuatorAt(_actuatorScreen).state ? "Apagar?"
                                                             : "Encender?");
      break;
    case State::SCHEDULE_ACTUATOR_CONNECTION:
      _showScheduleActuatorConnection();
      break;
    case State::SCHEDULE_ACTUATOR_DESCONNECTION:
      _showScheduleActuatorDesconnection();
      break;
    case State::SCHEDULE_ACTUATOR_CONFIRM:
      _showConfirm("Programar?");
      break;
    case State::CONFIG:
      _showConfig();
      break;
    case State::CONFIG_CONFIRM:
      _showConfirm("Iniciar hotspot?");
      break;
    case State::CONFIG_EXIT_CONFIRM:
      _showConfirm("Desea salir?");
      break;
    case State::ABOUT:
      _showAbout();
      break;
  }
}

void MenuManager::showCustomInfoScreen(const String& line1,
                                       const String& line2) {
  _stopKeypad = true;
  _lcd.clear();
  _lcd.setCursor(0, 0);
  _lcd.print(line1.c_str());
  _lcd.setCursor(0, 1);
  _lcd.print(line2.c_str());
}

void MenuManager::clearCustomInfoScreen() {
  _stopKeypad = false;
  updateDisplay();
}

void MenuManager::tryConnect(const String& ssid) {
  _lcd.clear();

  _lcd.setCursor(0, 0);
  _lcd.print("Conectando a");

  for (int i = 0; i < 16; i++) {
    _lcd.setCursor(i, 1);

    if (ssid.length() > 16 && (i == 13 || i == 14 || i == 15)) {
      _lcd.print(".");
    } else
      _lcd.print(ssid.charAt(i));
  }
}

void MenuManager::on(Event event, std::function<void()> callback) {
  _callbacks[event] = callback;
}

void MenuManager::updateData() {
  switch (_currentState) {
    case State::SENSOR:
      _lcd.clear();
      _showSensor();
      break;
    case State::ACTUATOR:
      _lcd.clear();
      _showActuator();

    default:
      break;
  }
}

void MenuManager::_trigger(Event event) {
  auto it = _callbacks.find(event);

  if (it != _callbacks.end() && it->second) {
    it->second();
  }
}

void MenuManager::_showMain() {
  for (int i = 0; i < 2; i++) {
    int idx = _displayStart + i;
    if (idx >= MAIN_MENU_COUNT) break;

    _lcd.setCursor(0, i);
    _lcd.print(idx == _menuPosition ? ">" : " ");
    _lcd.setCursor(1, i);
    _lcd.print(_mainMenuItems[idx].text);
  }
}

void MenuManager::_showStatus() {
  _lcd.setCursor(0, 0);
  _lcd.printf("Wifi: %s", formatBooleanToText(_data.wifi).c_str());
  _lcd.setCursor(0, 1);
  _lcd.printf("Internet: %s", formatBooleanToText(_data.internet).c_str());
}

void MenuManager::_showConfig() {
  _lcd.setCursor(0, 0);
  _lcd.print("Hotspot");
  _lcd.setCursor(0, 1);
  _lcd.printf("IP: %s", _data.ipAP.c_str());
}

void MenuManager::_showAbout() {
  _lcd.setCursor(0, 0);
  _lcd.print("ESP32 System");
  _lcd.setCursor(0, 1);
  _lcd.print("@vircoding");
}

void MenuManager::_showSensor() {
  const size_t sensorListSize = _now.getSensorListSize();

  if (sensorListSize > 0) {
    if (_sensorScreen >= 0 && _sensorScreen < sensorListSize) {
      NowManager::SensorData data = _now.getSensorAt(_sensorScreen);
      bool isValid;

      _lcd.setCursor(0, 0);
      _lcd.print(data.deviceName.c_str());
      _lcd.setCursor(0, 1);

      switch (data.type) {
        case NowManager::SensorValueType::BOOL:
          _lcd.printf("%s: %s", data.variable.c_str(),
                      data.isConnected
                          ? formatBooleanToText(data.value.b).c_str()
                          : "Desc");
          break;

        case NowManager::SensorValueType::INT:
          isValid = !isnan(data.value.i);
          _lcd.printf(
              "%s: %s%s", data.variable.c_str(),
              data.isConnected ? (isValid ? String(data.value.i).c_str() : "")
                               : "Desc",
              data.isConnected ? (isValid ? data.units.c_str() : "") : "");
          break;

        case NowManager::SensorValueType::FLOAT:
          isValid = !isnan(data.value.f);
          _lcd.printf(
              "%s: %s%s", data.variable.c_str(),
              data.isConnected
                  ? (isValid ? String(round(data.value.f * 10) / 10).c_str()
                             : "")
                  : "Desc",
              data.isConnected ? (isValid ? data.units.c_str() : "") : "");
          break;
      }
    }
  } else {
    _lcd.setCursor(0, 0);
    _lcd.print("Sin sensores");
    _lcd.setCursor(0, 1);
    _lcd.print("vinculados");
  }
}

void MenuManager::_showActuator() {
  const size_t actuatorListSize = _now.getActuatorListSize();

  if (actuatorListSize > 0) {
    if (_actuatorScreen >= 0 && _actuatorScreen < actuatorListSize) {
      NowManager::ActuatorData data = _now.getActuatorAt(_actuatorScreen);

      _lcd.setCursor(0, 0);
      _lcd.print(data.deviceName.c_str());

      if (data.isConnected) {
        _lcd.setCursor(1, 1);
        _lcd.printf("%s", data.state ? "ON" : "OFF");
        _lcd.setCursor(7, 1);
        _lcd.print("Programar");

        // Cursor dinámico
        _lcd.setCursor(0, 1);
        _lcd.print(_actuatorOption == 0 ? ">" : " ");

        _lcd.setCursor(6, 1);
        _lcd.print(_actuatorOption == 1 ? ">" : " ");
      } else {
        _lcd.setCursor(0, 1);
        _lcd.print("Desconectado");
      }
    }
  } else {
    _lcd.setCursor(0, 0);
    _lcd.print("Sin actuadores");
    _lcd.setCursor(0, 1);
    _lcd.print("vinculados");
  }
}

void MenuManager::_showScheduleActuatorConnection() {
  _lcd.setCursor(1, 0);
  _lcd.printf(
      "%s",
      _scheduleActuatorConnectionTimes[_scheduleActuatorConnectionDisplayStart]
          .text);
  _lcd.setCursor(8, 0);
  _lcd.printf(
      "%s",
      _scheduleActuatorConnectionTimes[_scheduleActuatorConnectionDisplayStart +
                                       1]
          .text);
  _lcd.setCursor(1, 1);
  _lcd.printf(
      "%s",
      _scheduleActuatorConnectionTimes[_scheduleActuatorConnectionDisplayStart +
                                       2]
          .text);
  _lcd.setCursor(8, 1);
  _lcd.printf(
      "%s",
      _scheduleActuatorConnectionTimes[_scheduleActuatorConnectionDisplayStart +
                                       3]
          .text);

  // Cursor dinámico
  _lcd.setCursor(0, 0);
  _lcd.print(_scheduleActuatorConnectionOption == 0 ? ">" : " ");

  _lcd.setCursor(7, 0);
  _lcd.print(_scheduleActuatorConnectionOption == 1 ? ">" : " ");

  _lcd.setCursor(0, 1);
  _lcd.print(_scheduleActuatorConnectionOption == 2 ? ">" : " ");

  _lcd.setCursor(7, 1);
  _lcd.print(_scheduleActuatorConnectionOption == 3 ? ">" : " ");
}

void MenuManager::_showScheduleActuatorDesconnection() {
  _lcd.setCursor(1, 0);
  _lcd.printf("%s", _scheduleActuatorDesconnectionTimes
                        [_scheduleActuatorDesconnectionDisplayStart]
                            .text);
  _lcd.setCursor(8, 0);
  _lcd.printf("%s", _scheduleActuatorDesconnectionTimes
                        [_scheduleActuatorDesconnectionDisplayStart + 1]
                            .text);
  _lcd.setCursor(1, 1);
  _lcd.printf("%s", _scheduleActuatorDesconnectionTimes
                        [_scheduleActuatorDesconnectionDisplayStart + 2]
                            .text);
  _lcd.setCursor(8, 1);
  _lcd.printf("%s", _scheduleActuatorDesconnectionTimes
                        [_scheduleActuatorDesconnectionDisplayStart + 3]
                            .text);

  // Cursor dinámico
  _lcd.setCursor(0, 0);
  _lcd.print(_scheduleActuatorDesconnectionOption == 0 ? ">" : " ");

  _lcd.setCursor(7, 0);
  _lcd.print(_scheduleActuatorDesconnectionOption == 1 ? ">" : " ");

  _lcd.setCursor(0, 1);
  _lcd.print(_scheduleActuatorDesconnectionOption == 2 ? ">" : " ");

  _lcd.setCursor(7, 1);
  _lcd.print(_scheduleActuatorDesconnectionOption == 3 ? ">" : " ");
}

void MenuManager::_showConfirm(const String& message) {
  _lcd.setCursor(0, 0);
  _lcd.print(message.c_str());

  _lcd.setCursor(0, 1);
  _lcd.print("    No     Si  ");

  // Cursor dinámico
  _lcd.setCursor(3, 1);
  _lcd.print(_confirmOption == 0 ? ">" : " ");

  _lcd.setCursor(10, 1);
  _lcd.print(_confirmOption == 1 ? ">" : " ");
}