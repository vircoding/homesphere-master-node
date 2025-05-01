#pragma once

#include <ArduinoJson.h>

class ConfigManager {
 public:
  struct NetworkConfig {
    String ssid;
    String password;
  };

  bool init();
  bool saveSTAConfig(const String& ssid, const String& password);

  NetworkConfig getAPConfig() const { return _apConfig; }
  NetworkConfig getSTAConfig() const { return _staConfig; }
  NetworkConfig getMeshConfig() const { return _meshConfig; }

 private:
  NetworkConfig _apConfig;
  NetworkConfig _staConfig;
  NetworkConfig _meshConfig;

  bool _loadConfig();
  bool _writeConfig(const JsonDocument& doc);
};