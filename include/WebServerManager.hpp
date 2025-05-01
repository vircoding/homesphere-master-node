#pragma once

#include <ESPAsyncWebServer.h>

#include "ConfigManager.hpp"
#include "WiFiManager.hpp"

class WebServerManager {
 public:
  WebServerManager(ConfigManager& config, WiFiManager& wifi);
  String begin();
  void end();
  void setupRoutes();
  bool getIsListening() const { return _isListening; }

 private:
  bool _isListening;
  AsyncWebServer _server{80};
  ConfigManager& _config;
  WiFiManager& _wifi;
  ConfigManager::NetworkConfig _partialConfig;
};