#include "MenuManager.hpp"

#include <cmath>

#include "KeypadManager.hpp"
#include "Utils.hpp"

MenuManager::MenuManager(uint8_t lcdRS, uint8_t lcdEN, uint8_t lcdD4,
                         uint8_t lcdD5, uint8_t lcdD6, uint8_t lcdD7,
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
        if (_now.getSensorListSize() > 0) {
          _sensorScreen = (_sensorScreen + 1) % _now.getSensorListSize();
        }
      } else if (key == Key::BACK) {
        _currentState = State::MAIN;
        _sensorScreen = 0;
      }
      break;

    case State::ACTION:
      if (key == Key::UP || key == Key::DOWN) {
        _actuatorScreen = (_actuatorScreen + 1) % 2;
      } else if (key == Key::BACK) {
        _currentState = State::MAIN;
        _actuatorScreen = 0;
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
    case State::ACTION:
      _showAction();
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
    if (_sensorScreen >= 0 && _sensorScreen < _now.getSensorListSize()) {
      NowManager::SensorData data = _now.getSensorAt(_sensorScreen);
      bool isValid;

      _lcd.setCursor(0, 0);
      _lcd.print(data.deviceName);
      _lcd.setCursor(0, 1);

      switch (data.type) {
        case NowManager::SensorValueType::BOOL:
          _lcd.printf("%s: %s", data.variable.c_str(),
                      formatBooleanToText(data.value.b).c_str());
          break;

        case NowManager::SensorValueType::INT:
          isValid = !isnan(data.value.i);
          _lcd.printf("%s: %s%s", data.variable.c_str(),
                      isValid ? String(data.value.i).c_str() : "",
                      isValid ? data.units.c_str() : "");
          break;

        case NowManager::SensorValueType::FLOAT:
          isValid = !isnan(data.value.f);
          _lcd.printf(
              "%s: %s%s", data.variable.c_str(),
              isValid ? String(round(data.value.f * 10) / 10).c_str() : "",
              isValid ? data.units.c_str() : "");
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

void MenuManager::_showAction() {
  if (_actuatorScreen == 0) {
    _lcd.print("Motor");
    _lcd.setCursor(0, 1);
    _lcd.print("RUN");
  } else {
    _lcd.print("Luces");
    _lcd.setCursor(0, 1);
    _lcd.print("RUN");
  }
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