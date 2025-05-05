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
  bool saveNodeConfig(const uint8_t* mac, const uint8_t nodeType,
                      const uint8_t* firmwareVersion);
  NetworkConfig getAPConfig() const { return _apConfig; }
  NetworkConfig getSTAConfig() const { return _staConfig; }

 private:
  NetworkConfig _apConfig;
  NetworkConfig _staConfig;

  bool _loadConfig();
  bool _writeConfig(const JsonDocument& doc);
};