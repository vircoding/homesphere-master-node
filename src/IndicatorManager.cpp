#include "IndicatorManager.hpp"

IndicatorManager::IndicatorManager(uint8_t redPin, uint8_t greenPin,
                                   uint8_t bluePin)
    : _RED_PIN(redPin), _GREEN_PIN(greenPin), _BLUE_PIN(bluePin) {}

void IndicatorManager::begin() {
  // Configurar PWM a 8 bits de resolución y frecuencia adecuada (ej: 5000Hz).
  ledcSetup(0, 5000, 8);  // Canal 0, 5kHz, 8 bits
  ledcSetup(1, 5000, 8);  // Canal 1
  ledcSetup(2, 5000, 8);  // Canal 2

  ledcAttachPin(_RED_PIN, 0);    // Red en canal 0
  ledcAttachPin(_GREEN_PIN, 1);  // Green en canal 1
  ledcAttachPin(_BLUE_PIN, 2);   // Blue en canal 2
}

void IndicatorManager::setIndicator(Status status) {
  switch (status) {
    case Status::ONLINE:
      _setColor(0, 0, 255);
      break;
    case Status::OFFLINE:
      _setColor(0, 255, 0);
      break;
    case Status::PENDING:
      _setColor(0, 255, 255);
      break;
    case Status::OFF:
      _setColor(0, 0, 0);
      break;
    case Status::ERROR:
    default:
      _setColor(255, 0, 0);
      break;
  }
}

void IndicatorManager::_setColor(uint8_t red, uint8_t green, uint8_t blue) {
  ledcWrite(0, red);
  ledcWrite(1, green);
  ledcWrite(2, blue);
}