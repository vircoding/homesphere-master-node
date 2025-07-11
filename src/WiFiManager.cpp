#include "WiFiManager.hpp"

#include "Utils.hpp"

void WiFiManager::modeAPSTA() { WiFi.mode(WIFI_AP_STA); }

String WiFiManager::startAP(const String& ssid, const String& password) {
  WiFi.softAP(ssid.c_str(), password.c_str());
  return WiFi.softAPIP().toString();
}

void WiFiManager::closeAP() { WiFi.softAPdisconnect(); }

bool WiFiManager::connectSTA(const String& ssid, const String& password) {
  Serial.printf("Network: %s\n", ssid.c_str());
  Serial.printf("Password: %s\n", password.c_str());

  // Implementación de la conexión STA
  WiFi.begin(ssid.c_str(), password.c_str());

  // Esperar a que se establezca la conexión
  uint8_t retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 20) {
    delay(500);
    Serial.print(".");
    retries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConectado a WiFi!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    return true;
  }

  Serial.println("\nError al conectar a WiFi");
  return false;
}

void WiFiManager::scanNetworks() {
  _scanResults.clear();
  uint16_t numNetworks =
      WiFi.scanNetworks(/*async*/ false, /*hidden*/ false, /*passive*/ false,
                        /*max_ms_per_chan*/ 300);

  uint8_t i = 0;
  for (i; i < numNetworks; i++) {
    ScanResult result;
    result.ssid = WiFi.SSID(i);
    result.rssi = WiFi.RSSI(i);
    result.secure = isNetworkSecure(WiFi.encryptionType(i));
    _scanResults.push_back(result);
  }

  WiFi.scanDelete();
}

const std::vector<WiFiManager::ScanResult>& WiFiManager::getScanResults()
    const {
  return _scanResults;
}