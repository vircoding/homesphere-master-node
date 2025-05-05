#include "ConfigManager.hpp"

#include <LittleFS.h>

bool ConfigManager::init() {
  if (!LittleFS.begin()) return false;
  return _loadConfig();
}

bool ConfigManager::_loadConfig() {
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) return false;

  JsonDocument doc;
  if (deserializeJson(doc, configFile)) {
    configFile.close();
    return false;
  }

  _apConfig.ssid = doc["ap_ssid"].as<String>();
  _apConfig.password = doc["ap_password"].as<String>();
  _staConfig.ssid = doc["sta_ssid"].as<String>();
  _staConfig.password = doc["sta_password"].as<String>();

  configFile.close();
  return true;
}

bool ConfigManager::saveSTAConfig(const String& ssid, const String& password) {
  File configFile = LittleFS.open("/config.json", "r");
  JsonDocument doc;

  if (deserializeJson(doc, configFile)) {
    configFile.close();
    return false;
  }

  configFile.close();
  doc["sta_ssid"] = ssid;
  doc["sta_password"] = password;

  return _writeConfig(doc);
}

bool ConfigManager::saveNodeConfig(const uint8_t* mac, const uint8_t nodeType,
                                   const uint8_t* firmwareVersion) {
  const size_t capacity =
      2 * JSON_ARRAY_SIZE(12) + JSON_OBJECT_SIZE(1) + 12 * JSON_OBJECT_SIZE(4);

  File configFile = LittleFS.open("/config.json", "r");
  JsonDocument doc;

  if (deserializeJson(doc, configFile)) {
    configFile.close();
    return false;
  }

  configFile.close();
  // doc["nodes"] =
}

bool ConfigManager::_writeConfig(const JsonDocument& doc) {
  File configFile = LittleFS.open("/config.json", "w");
  if (!configFile) return false;

  serializeJsonPretty(doc, configFile);
  configFile.close();
  return true;
}