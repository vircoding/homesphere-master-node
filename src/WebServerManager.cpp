#include "WebServerManager.hpp"

#include <LittleFS.h>
#include <freertos/FreeRTOS.h>

#include "Utils.hpp"

WebServerManager::WebServerManager(ConfigManager& config, WiFiManager& wifi)
    : _config(config), _wifi(wifi) {}

String WebServerManager::begin() {
  const ConfigManager::NetworkConfig apConfig = _config.getAPConfig();
  const String ip = _wifi.startAP(apConfig.ssid, apConfig.password);

  _server.begin();
  _isListening = true;

  return ip;
}

void WebServerManager::end() {
  _wifi.closeAP();

  _server.end();
  _isListening = false;
}

void WebServerManager::setupRoutes() {
  // Scan Networks
  _server.on("/scan", HTTP_GET, [this](AsyncWebServerRequest* request) {
    _wifi.scanNetworks();
    const auto& results = _wifi.getScanResults();

    JsonDocument doc;
    JsonArray networks = doc.to<JsonArray>();

    for (const auto& result : results) {
      JsonObject network = networks.add<JsonObject>();
      network["ssid"] = result.ssid;
      network["rssi"] = result.rssi;
      network["secure"] = result.secure;
    }

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // Presave wifi settings
  _server.on(
      "/setup/wifi", HTTP_POST,
      [this](AsyncWebServerRequest* request) {
        // Verificar si hay datos en _tempObject
        if (request->_tempObject == nullptr) {
          request->send(400, "text/plain", "Cuerpo vacío");
          return;
        }

        String* body = (String*)request->_tempObject;

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, *body);

        if (error) {
          request->send(400, "text/plain", "Error en el formato JSON");
          delete body;
          return;
        }

        String ssid = doc["ssid"].as<String>();
        String password = doc["password"].as<String>();

        // Sanitizar entradas
        sanitizeInput(ssid);
        sanitizeInput(password, 64);  // Mayor longitud para contraseñas

        // Validación básica
        if (ssid.isEmpty()) {
          request->send(400, "text/plain", "SSID inválido");
          delete body;
          return;
        }

        _partialConfig = {ssid, password};

        request->send(200, "text/plain", "Configuración recibida");
        delete body;
        request->_tempObject = nullptr;
      },
      nullptr,
      [](AsyncWebServerRequest* request, uint8_t* data, size_t len,
         size_t index, size_t total) {
        if (index == 0) request->_tempObject = new String();
        ((String*)request->_tempObject)->concat((const char*)data, len);
      });

  // Confirm settings
  _server.on("/setup", HTTP_POST, [this](AsyncWebServerRequest* request) {
    if (!_config.saveSTAConfig(_partialConfig.ssid, _partialConfig.password)) {
      request->send(500, "text/plain", "Error de servidor");
      return;
    }

    request->send(200, "text/plain", "Configuración guardada");

    // Delay - 100ms
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP.restart();
  });

  _server.on("/close", HTTP_POST, [this](AsyncWebServerRequest* request) {
    _server.end();

    request->send(200, "text/plain", "Sistema reiniciado");

    // Delay - 100ms
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP.restart();
  });

  // Static Files
  _server.serveStatic("/", LittleFS, "/www/").setDefaultFile("index.html");
  // .setCacheControl("max-age=3600");

  // 404
  _server.onNotFound([](AsyncWebServerRequest* request) {
    // Si la URL no es un archivo estático, servir index.html
    if (!LittleFS.exists("/www" + request->url())) {
      request->send(LittleFS, "/www/index.html", "text/html");
    } else {
      request->send(404);
    }
  });
}