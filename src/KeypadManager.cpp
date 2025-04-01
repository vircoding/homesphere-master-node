#include <KeypadManager.hpp>

KeypadManager::KeypadManager(uint8_t upPin, uint8_t downPin, uint8_t backPin,
                             uint8_t enterPin)
    : _UP_PIN(upPin),
      _DOWN_PIN(downPin),
      _BACK_PIN(backPin),
      _ENTER_PIN(enterPin) {}

void KeypadManager::begin() {
  _upDebouncer.attach(_UP_PIN, INPUT_PULLUP);
  _downDebouncer.attach(_DOWN_PIN, INPUT_PULLUP);
  _backDebouncer.attach(_BACK_PIN, INPUT_PULLUP);
  _enterDebouncer.attach(_ENTER_PIN, INPUT_PULLUP);

  _upDebouncer.interval(50);
  _downDebouncer.interval(50);
  _backDebouncer.interval(50);
  _enterDebouncer.interval(50);
}

Key KeypadManager::getKey() {
  _upDebouncer.update();
  _downDebouncer.update();
  _backDebouncer.update();
  _enterDebouncer.update();

  if (_upDebouncer.fell()) return Key::UP;
  if (_downDebouncer.fell()) return Key::DOWN;
  if (_backDebouncer.fell()) return Key::BACK;
  if (_enterDebouncer.fell()) return Key::ENTER;

  return Key::NONE;
}