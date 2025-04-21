#pragma once

#include <Arduino.h>
#include <WiFi.h>

#include <vector>

class WiFiManager {
 public:
  struct ScanResult {
    String ssid;
    int32_t rssi;
    bool secure;
  };

  void modeAPSTA();
  String startAP(const String& ssid, const String& password);
  void closeAP();
  bool connectSTA(const String& ssid, const String& password);
  void scanNetworks();
  const std::vector<ScanResult>& getScanResults() const;

 private:
  std::vector<ScanResult> _scanResults;
};