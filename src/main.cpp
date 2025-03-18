#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <esp_task_wdt.h>

int LED = 2;

// Configuración del hotspot
String apSSID;
String apPassword;

bool hasInternet = true;

// Configuración de la red
String staSSID;
String staPassword;

// Crear el servidor web asíncrono en el puerto 80
AsyncWebServer server(80);

bool initConfig() {
  // Abrir archivo config.json desde LittleFS
  File configFile = LittleFS.open("/config.json", "r");

  if (!configFile) {
    Serial.println("Error al abrir el archivo de configuración");
    return false;
  }

  // Parsear el JSON
  JsonDocument jsonDoc;
  DeserializationError error = deserializeJson(jsonDoc, configFile);

  if (error) {
    Serial.println("Error al parsear el JSON");
    configFile.close();
    return false;
  }

  // Extraer los valores del JSON
  apSSID = jsonDoc["ap_ssid"].as<String>();
  apPassword = jsonDoc["ap_password"].as<String>();
  staSSID = jsonDoc["sta_ssid"].as<String>();
  staPassword = jsonDoc["sta_password"].as<String>();

  // Cierra el archivo config.json
  configFile.close();
  return true;
}

bool isNetworkSecure(wifi_auth_mode_t encryptionType) {
  // Devuelve true si la red está protegida, false si es abierta
  return (encryptionType != WIFI_AUTH_OPEN);
}

void sanitizeInput(String &input, size_t maxLength = 32) {
  // Eliminar caracteres no ASCII y limitar longitud
  input.replace("\\", "");
  input.replace("\"", "");
  input.replace("'", "");
  input.replace(";", "");
  if (input.length() > maxLength) input = input.substring(0, maxLength);
}

void saveWiFiCredentials(const String &ssid, const String &password) {
  // Leer config.json existente
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Error al leer configuración");
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();

  if (error) {
    Serial.println("Error parseando configuración");
    return;
  }

  // Actualizar valores
  doc["sta_ssid"] = ssid;
  doc["sta_password"] = password;

  // Escribir nuevo archivo
  configFile = LittleFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("Error al guardar configuración");
    return;
  }

  serializeJsonPretty(doc, configFile);
  configFile.close();
  Serial.println("Credenciales guardadas correctamente");
}

bool connectSTA() {
  // Intentar conectarse a la red
  WiFi.begin(staSSID, staPassword);
  Serial.print("Conectándose a ");
  Serial.println(staSSID);

  // Esperar a que se establezca la conexión
  unsigned int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  Serial.println("");

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Conexión establecida");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println("No se pudo conectar a la red");
    return false;
  }
}

void startWebServer() {
  // Configurar ESP32 como Access Point
  WiFi.softAP(apSSID.c_str(), apPassword.c_str());
  Serial.println("Modo Access Point activado");
  Serial.print("IP del hotspot: ");
  Serial.println(WiFi.softAPIP());

  // Ruta para escanear redes disponibles
  server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
    int networks = WiFi.scanNetworks(/*async=*/false, /*hidden=*/false,
                                     /*passive=*/false,
                                     /*max_ms_per_chan=*/300);

    JsonDocument doc;
    JsonArray networksArray = doc.to<JsonArray>();

    for (int i = 0; i < networks; i++) {
      JsonObject network = networksArray.add<JsonObject>();
      network["ssid"] = WiFi.SSID(i);
      network["rssi"] = WiFi.RSSI(i);
      network["secure"] =
          isNetworkSecure(WiFi.encryptionType(i));  // Cambiado a "secure"
    }

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);

    WiFi.scanDelete();
  });

  // Ruta para guardar credenciales de la red
  server.on(
      "/setup/wifi", HTTP_POST,
      [](AsyncWebServerRequest *request) {
        // Verificar si hay datos en _tempObject
        if (request->_tempObject == nullptr) {
          request->send(400, "text/plain", "Cuerpo vacío");
          return;
        }

        String *body = (String *)request->_tempObject;

        // Parsear JSON
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

        // Guardar en config.json
        saveWiFiCredentials(ssid, password);

        request->send(200, "text/plain", "Configuración guardada");
        delete body;
        request->_tempObject = nullptr;

        ESP.restart();
      },
      nullptr,
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len,
         size_t index, size_t total) {
        // Acumular el cuerpo usando _tempObject
        if (index == 0) {
          request->_tempObject = new String();
        }

        String *body = (String *)request->_tempObject;
        body->concat((const char *)data, len);
      });

  // Servir archivos estáticos
  server.serveStatic("/", LittleFS, "/www/")
      .setDefaultFile("index.html")
      .setCacheControl("max-age=3600");

  // Manejar errores 404
  server.onNotFound([](AsyncWebServerRequest *request) {
    // Servir el archivo 404.html desde LittleFA
    request->send(LittleFS, "/www/404.html", "text/html");
  });

  // Iniciar el servidor
  server.begin();
  Serial.println("Servidor web iniciado");
}

void setup() {
  esp_task_wdt_init(10, false);  // 10 segundos (ajusta según necesites)
  Serial.begin(115200);

  pinMode(LED, OUTPUT);

  // Inicializar LittleFS
  if (!LittleFS.begin()) {
    Serial.println("Error al montar LittleFS");
    ESP.restart();
  }
  Serial.println("LittleFS montado correctamente");

  // Inicializar la configuración
  if (!initConfig()) {
    Serial.println("Error al cargar la configuración");
    ESP.restart();
  } else
    Serial.println("Configuración cargada correctamente");

  if (!connectSTA()) {
    startWebServer();
  } else {
    digitalWrite(LED, HIGH);
  }
}

void loop() {}