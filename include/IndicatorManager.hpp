#pragma once

#include <Arduino.h>

enum class Status {
  ONLINE,
  OFFLINE,
  PENDING,
  ERROR,
  OFF,
};

class IndicatorManager {
 public:
  IndicatorManager(const gpio_num_t redPin, const gpio_num_t greenPin,
                   const gpio_num_t bluePin);
  void begin();
  void set(Status status);

 private:
  const gpio_num_t _RED_PIN;
  const gpio_num_t _GREEN_PIN;
  const gpio_num_t _BLUE_PIN;

  // Métodos privados
  void _setColor(uint8_t red, uint8_t green, uint8_t blue);
};