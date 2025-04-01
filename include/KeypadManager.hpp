#pragma once

#include <Bounce2.h>

enum class Key { UP, DOWN, BACK, ENTER, NONE };

class KeypadManager {
 public:
  KeypadManager(uint8_t upPin, uint8_t downPin, uint8_t backPin,
                uint8_t enterPin);
  void begin();
  Key getKey();

 private:
  Bounce _upDebouncer;
  Bounce _downDebouncer;
  Bounce _backDebouncer;
  Bounce _enterDebouncer;

  const uint8_t _UP_PIN;
  const uint8_t _DOWN_PIN;
  const uint8_t _BACK_PIN;
  const uint8_t _ENTER_PIN;
};