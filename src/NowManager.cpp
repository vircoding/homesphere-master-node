#include "NowManager.hpp"

#include <WiFi.h>

#include "Utils.hpp"

bool NowManager::init() {
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error inicializando ESP-NOW");
    return false;
  }

  return true;
}

bool NowManager::stop() {
  if (reset() && esp_now_deinit() != ESP_OK) return false;

  return true;
}

bool NowManager::reset() {
  // Eliminar todos los peers
  for (auto& device : _pairedDevices) {
    if (!removeDevice(device.mac)) {
      Serial.printf("Error eliminando peer %s\n",
                    macToString(device.mac).c_str());
      return false;
    }
  }

  // Eliminar broadcast peer si existe
  if (_isBroadcastPeerRegistered) {
    if (esp_now_del_peer(_broadcastMac) != ESP_OK) {
      Serial.printf("Error eliminando peer %s\n",
                    macToString(_broadcastMac).c_str());
      return false;
    }

    _isBroadcastPeerRegistered = false;
  }

  // Desregistrar callbacks
  if (esp_now_unregister_recv_cb() != ESP_OK) {
    Serial.println("Error desregistrando callback RX");
    return false;
  }

  if (esp_now_unregister_send_cb() != ESP_OK) {
    Serial.println("Error desregistrando callback TX");
    return false;
  }

  return true;
}

bool NowManager::_registerPeer(const uint8_t* mac) {
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, mac, 6);
  peerInfo.channel = 0;
  peerInfo.ifidx = WIFI_IF_STA;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Error registrando peer");
    return false;
  }

  return true;
}

bool NowManager::registerBroadcastPeer() {
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, _broadcastMac, 6);
  peerInfo.channel = 0;
  peerInfo.ifidx = WIFI_IF_STA;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Error registrando peer");
    return false;
  }

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

    default:
      return 0;  // Tipo desconocido
  }
}

bool NowManager::addDevice(const uint8_t* mac, const uint8_t nodeType,
                           const uint8_t* firmwareVersion) {
  // Verificar si ya existe
  auto it = std::find_if(
      _pairedDevices.begin(), _pairedDevices.end(),
      [&mac](const DeviceInfo& d) { return memcmp(d.mac, mac, 6) == 0; });

  if (it != _pairedDevices.end()) {
    Serial.println("Dispositivo ya registrado");
    return false;
  }

  if (_pairedDevices.size() >= 12) return false;

  // Añadir nuevo nodo
  DeviceInfo newDevice;
  memcpy(newDevice.mac, mac, 6);
  memcpy(newDevice.firmwareVersion, firmwareVersion, 3);
  newDevice.nodeType = nodeType;
  newDevice.lastSeen = millis();

  _pairedDevices.push_back(newDevice);

  // Registrar Peer
  if (!_registerPeer(mac)) {
    _pairedDevices.pop_back();
    return false;
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
    // esp_err_t result;
    // result = esp_now_del_peer(mac);

    if (esp_now_del_peer(mac) != ESP_OK) {
      Serial.println("Error en la eliminacion del peer");
      return false;
    }
    _pairedDevices.erase(it);

    return true;
  }

  return false;
}
