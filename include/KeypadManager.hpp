#pragma once

#include <Bounce2.h>

enum class Key { UP, DOWN, BACK, ENTER, NONE };

class KeypadManager {
 public:
  KeypadManager(const gpio_num_t upPin, const gpio_num_t downPin,
                const gpio_num_t backPin, const gpio_num_t enterPin);
  void begin();
  Key getKey();

 private:
  static constexpr uint8_t DEBOUNCE_INTERVAL = 50;

  Bounce _upDebouncer;
  Bounce _downDebouncer;
  Bounce _backDebouncer;
  Bounce _enterDebouncer;

  const gpio_num_t _UP_PIN;
  const gpio_num_t _DOWN_PIN;
  const gpio_num_t _BACK_PIN;
  const gpio_num_t _ENTER_PIN;
};