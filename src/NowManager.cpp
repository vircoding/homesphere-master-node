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
  esp_err_t result;

  // Eliminar todos los peers
  for (auto& device : _pairedDevices) {
    Serial.println("Hereeeeeee 1");

    result = esp_now_del_peer(device.mac);
    if (result != ESP_OK) {
      Serial.printf("Error eliminando peer %s: %s\n", macToString(device.mac),
                    esp_err_to_name(result));
      return false;
    }
  }

  // Eliminar masterPeer si existe
  if (_isBroadcastPeerRegistered) {
    Serial.println("Hereeeeeee 2");

    result = esp_now_del_peer(_broadcastMac);
    if (result != ESP_OK) {
      Serial.printf("Error eliminando peer %s: %s\n",
                    macToString(_broadcastMac), esp_err_to_name(result));
      return false;
    }

    _isBroadcastPeerRegistered = false;
  }

  // Desregistrar callbacks
  result = esp_now_unregister_recv_cb();
  if (result != ESP_OK) {
    Serial.printf("Error desregistrando callback RX: %s\n",
                  esp_err_to_name(result));
    return false;
  }

  result = esp_now_unregister_send_cb();
  if (result != ESP_OK) {
    Serial.printf("Error desregistrando callback TX: %s\n",
                  esp_err_to_name(result));
    return false;
  }

  return true;
}

bool NowManager::registerPeer(const uint8_t* mac) {
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

bool NowManager::_addDevice(const uint8_t* mac, uint8_t nodeType,
                            uint8_t* firmwareVersion) {
  // Verificar si ya existe
  auto it = std::find_if(
      _pairedDevices.begin(), _pairedDevices.end(),
      [&mac](const DeviceInfo& d) { return memcmp(d.mac, mac, 6) == 0; });

  if (it != _pairedDevices.end()) {
    Serial.println("Dispositivo ya registrado");
    return false;
  }

  if (_pairedDevices.size() < 12) return false;

  // Añadir nuevo nodo
  DeviceInfo newDevice;
  memcpy(newDevice.mac, mac, 6);
  memcpy(newDevice.firmwareVersion, firmwareVersion, 3);
  newDevice.nodeType = nodeType;
  newDevice.lastSeen = millis();

  _pairedDevices.push_back(newDevice);

  return true;
}
