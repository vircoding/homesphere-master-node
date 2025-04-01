#pragma once

#include <ESPAsyncWebServer.h>

#include "ConfigManager.hpp"
#include "WiFiManager.hpp"

class WebServerManager {
 public:
  struct PartialConfig {
    String ssid;
    String password;
  };

  WebServerManager(ConfigManager& configManager, WiFiManager& wifiManager);
  String begin();
  void end();
  void setupRoutes();
  bool getIsListening() const { return _isListening; }

 private:
  bool _isListening;
  AsyncWebServer _server{80};
  ConfigManager& _config;
  WiFiManager& _wifi;
  PartialConfig _partialConfig;
};