#include "ConfigManager.hpp"

#include <LittleFS.h>

#include "Utils.hpp"

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

  JsonArray nodes = doc["nodes"].as<JsonArray>();
  for (JsonObject node : nodes) {
    NodeInfo newNode;
    stringToMac(node["mac"], newNode.mac);
    newNode.nodeType = node["node_type"].as<uint8_t>();
    newNode.deviceName = node["device_name"].as<String>();
    if (!stringToFirmwareVersion(node["firmware_version"],
                                 newNode.firmwareVersion)) {
      newNode.firmwareVersion[0] = 0;
      newNode.firmwareVersion[1] = 0;
      newNode.firmwareVersion[2] = 0;
    }

    _nodes.push_back(newNode);
  }

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
  File configFile = LittleFS.open("/config.json", "r");
  JsonDocument doc;

  if (deserializeJson(doc, configFile)) {
    configFile.close();
    return false;
  }

  configFile.close();
  JsonArray nodes = doc["nodes"].as<JsonArray>();

  // Buscar si ya existe el nodo
  bool exists = false;
  String macStr = macToString(mac);

  for (JsonObject node : nodes) {
    if (node["mac"] == macStr) {
      exists = true;
      node["firmware_version"] = firmwareVersionToString(firmwareVersion);
      break;
    }
  }

  // Add new node (max 12)
  if (!exists && nodes.size() < 12) {
    JsonObject newNode = nodes.add<JsonObject>();
    newNode["mac"] = macStr;
    newNode["node_type"] = nodeType;
    newNode["device_name"] = "Nodo Secundario";
    newNode["firmware_version"] = firmwareVersionToString(firmwareVersion);
  }

  if (!_writeConfig(doc)) return false;

  return _loadConfig();
}

void ConfigManager::printConfig() {
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) return;

  JsonDocument doc;
  if (deserializeJson(doc, configFile)) {
    configFile.close();
    return;
  }

  Serial.print("Config File: ");
  serializeJsonPretty(doc, Serial);
  Serial.println();
}

bool ConfigManager::_writeConfig(const JsonDocument& doc) {
  File configFile = LittleFS.open("/config.json", "w");
  if (!configFile) return false;

  serializeJsonPretty(doc, configFile);
  configFile.close();
  return true;
}