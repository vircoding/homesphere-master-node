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
  IndicatorManager(uint8_t redPin, uint8_t greenPin, uint8_t bluePin);
  void begin();
  void set(Status status);

 private:
  const uint8_t _RED_PIN;
  const uint8_t _GREEN_PIN;
  const uint8_t _BLUE_PIN;

  // Métodos privados
  void _setColor(uint8_t red, uint8_t green, uint8_t blue);
};