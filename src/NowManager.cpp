#include "NowManager.hpp"

#include <WiFi.h>

#include "Utils.hpp"

bool NowManager::init() {
  if (esp_now_init() != ESP_OK) return false;

  return true;
}

bool NowManager::stop() {
  if (reset() && esp_now_deinit() != ESP_OK) return false;

  return true;
}

bool NowManager::reset() {
  // Eliminar todos los peers
  for (auto& device : _pairedDevices) {
    if (!removeDevice(device.mac)) return false;
  }

  // Vaciar sensor list
  _sensors.clear();

  // Eliminar broadcast peer si existe
  if (_isBroadcastPeerRegistered) {
    if (esp_now_del_peer(_broadcastMac) != ESP_OK) return false;

    _isBroadcastPeerRegistered = false;
  }

  // Desregistrar callbacks
  if (esp_now_unregister_recv_cb() != ESP_OK ||
      esp_now_unregister_send_cb() != ESP_OK)
    return false;

  return true;
}

bool NowManager::_registerPeer(const uint8_t* mac) {
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, mac, 6);
  peerInfo.channel = 0;
  peerInfo.ifidx = WIFI_IF_STA;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) return false;

  return true;
}

bool NowManager::registerBroadcastPeer() {
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, _broadcastMac, 6);
  peerInfo.channel = 0;
  peerInfo.ifidx = WIFI_IF_STA;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) return false;

  _isBroadcastPeerRegistered = true;
  return true;
}

void NowManager::onSend(esp_now_send_cb_t callback) {
  esp_now_register_send_cb(callback);
}

void NowManager::unsuscribeOnSend() { esp_now_unregister_send_cb(); }

void NowManager::onReceived(esp_now_recv_cb_t callback) {
  esp_now_register_recv_cb(callback);
}

void NowManager::unsuscribeOnReceived() { esp_now_unregister_recv_cb(); }

bool NowManager::sendSyncBroadcastMsg() {
  NowManager::SyncBroadcastMsg msg;
  msg.pairingCode = esp_random();

  // Generate CRC8
  addCRC8(msg);

  return esp_now_send(_broadcastMac, (uint8_t*)&msg, sizeof(msg)) == ESP_OK;
}

bool NowManager::sendConfirmRegistrationMsg(const uint8_t* mac) {
  NowManager::ConfirmRegistrationMsg msg;

  return esp_now_send(mac, (uint8_t*)&msg, sizeof(msg)) == ESP_OK;
}

bool NowManager::validateMessage(MessageType expectedType, const uint8_t* data,
                                 size_t length) {
  // Evitar mensajes vacíos
  if (length < 1) return false;

  auto msgType = static_cast<MessageType>(data[0]);
  return (msgType == expectedType) && (length == _getMessageSize(expectedType));
}

size_t NowManager::_getMessageSize(MessageType type) {
  switch (type) {
    case MessageType::SYNC_BROADCAST:
      return sizeof(SyncBroadcastMsg);

    case MessageType::REGISTRATION:
      return sizeof(RegistrationMsg);

    case MessageType::CONFIRM_REGISTRATION:
      return sizeof(ConfirmRegistrationMsg);

    case MessageType::TEMPERATURE_HUMIDITY:
      return sizeof(TemperatureHumidityMsg);

    default:
      return 0;  // Tipo desconocido
  }
}

bool NowManager::addDevice(const uint8_t* mac, const uint8_t nodeType,
                           const String& deviceName,
                           const uint8_t* firmwareVersion) {
  // Verificar si ya existe
  auto it = std::find_if(
      _pairedDevices.begin(), _pairedDevices.end(),
      [&mac](const DeviceInfo& d) { return memcmp(d.mac, mac, 6) == 0; });

  if (it != _pairedDevices.end()) return false;

  if (_pairedDevices.size() >= 12) return false;

  // Añadir nuevo nodo
  DeviceInfo newDevice;
  memcpy(newDevice.mac, mac, 6);
  memcpy(newDevice.firmwareVersion, firmwareVersion, 3);
  newDevice.nodeType = nodeType;
  newDevice.lastSeen = millis();
  newDevice.deviceName = deviceName;

  _pairedDevices.push_back(newDevice);

  // Registrar Peer
  if (!_registerPeer(mac)) {
    _pairedDevices.pop_back();
    return false;
  }

  switch (static_cast<NodeType>(newDevice.nodeType)) {
    case NodeType::TEMPERATURE:
      SensorData data;
      memcpy(data.mac, mac, 6);
      data.deviceName = newDevice.deviceName;
      data.variable = "Temp";
      data.units = "C";
      data.type = SensorValueType::FLOAT;
      data.value.f = NAN;
      _sensors.push_back(data);
      data.variable = "Hum";
      data.units = "%";
      data.type = SensorValueType::FLOAT;
      data.value.f = NAN;
      _sensors.push_back(data);
      break;
  }

  return true;
}

void NowManager::printAllDevices() {
  int i = 0;
  Serial.println("Dispositivos vinculados: ");
  for (const auto& device : _pairedDevices) {
    Serial.printf("%d - MAC: %s, Tipo: %d, Ultima vez: %lu\n", i,
                  macToString(device.mac).c_str(), device.nodeType,
                  device.lastSeen);

    i++;
  }

  i = 0;
  Serial.println("Sensores vinculados: ");
  for (const auto& sensor : _sensors) {
    Serial.printf("%d - MAC: %s, Nombre: %s, Valor: %f\n", i,
                  macToString(sensor.mac).c_str(), sensor.deviceName.c_str(),
                  sensor.value);
    i++;
  }
}

NowManager::SensorData NowManager::getSensorAt(const int index) const {
  if (index >= 0 && index < _sensors.size()) {
    return _sensors.at(index);
  } else {
    SensorData data;
    data.type = SensorValueType::BOOL;
    data.value.b = false;
  }
}

void NowManager::updateSensorData(const uint8_t* mac, const String& variable,
                                  const bool value) {
  auto it = std::find_if(
      _sensors.begin(), _sensors.end(), [&mac, &variable](const SensorData& d) {
        return memcmp(d.mac, mac, 6) == 0 && d.variable == variable;
      });

  if (it != _sensors.end()) it.base()->value.b = value;
}

void NowManager::updateSensorData(const uint8_t* mac, const String& variable,
                                  const int value) {
  auto it = std::find_if(
      _sensors.begin(), _sensors.end(), [&mac, &variable](const SensorData& d) {
        return memcmp(d.mac, mac, 6) == 0 && d.variable == variable;
      });

  if (it != _sensors.end()) it.base()->value.i = value;
}

void NowManager::updateSensorData(const uint8_t* mac, const String& variable,
                                  const float value) {
  auto it = std::find_if(
      _sensors.begin(), _sensors.end(), [&mac, &variable](const SensorData& d) {
        return memcmp(d.mac, mac, 6) == 0 && d.variable == variable;
      });

  if (it != _sensors.end()) it.base()->value.f = value;
}

bool NowManager::isDevicePaired(const uint8_t* mac) {
  auto it = std::find_if(
      _pairedDevices.begin(), _pairedDevices.end(),
      [&mac](const DeviceInfo& d) { return memcmp(d.mac, mac, 6) == 0; });

  return it != _pairedDevices.end();
}

void NowManager::updateDeviceLastSeen(const uint8_t* mac) {
  auto it = std::find_if(
      _pairedDevices.begin(), _pairedDevices.end(),
      [&mac](const DeviceInfo& d) { return memcmp(d.mac, mac, 6) == 0; });

  if (it != _pairedDevices.end()) it.base()->lastSeen = millis();
}

NowManager::DeviceInfo* NowManager::findDevice(const uint8_t* mac) {
  auto it = std::find_if(
      _pairedDevices.begin(), _pairedDevices.end(),
      [&mac](const DeviceInfo& d) { return memcmp(d.mac, mac, 6) == 0; });

  return (it != _pairedDevices.end()) ? &(*it) : nullptr;
}

bool NowManager::removeDevice(const uint8_t* mac) {
  auto it = std::find_if(
      _pairedDevices.begin(), _pairedDevices.end(),
      [&mac](const DeviceInfo& d) { return memcmp(d.mac, mac, 6) == 0; });

  if (it != _pairedDevices.end()) {
    if (esp_now_del_peer(mac) != ESP_OK) return false;

    _pairedDevices.erase(it);

    return true;
  }

  return false;
}
