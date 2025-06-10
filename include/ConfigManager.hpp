#pragma once

#include <ArduinoJson.h>

#include <vector>

class ConfigManager {
 public:
  struct NetworkConfig {
    String ssid;
    String password;
  };

  struct NodeInfo {
    uint8_t mac[6];              // Dirección MAC
    uint8_t nodeType;            // Tipo de nodo
    String deviceName;           // Nombre del nodo
    uint8_t firmwareVersion[3];  // Version del firmware del nodo
  };

  bool init();
  bool saveSTAConfig(const String& ssid, const String& password);
  bool saveNodeConfig(const uint8_t* mac, const uint8_t nodeType,
                      const uint8_t* firmwareVersion);
  NetworkConfig getAPConfig() const { return _apConfig; }
  NetworkConfig getSTAConfig() const { return _staConfig; }
  NodeInfo getNode(const uint8_t index) { return _nodes[index]; }
  uint8_t getNodeLength() const { return _nodes.size(); }
  void printConfig();

 private:
  NetworkConfig _apConfig;
  NetworkConfig _staConfig;
  std::vector<NodeInfo> _nodes;

  bool _loadConfig();
  bool _writeConfig(const JsonDocument& doc);
};