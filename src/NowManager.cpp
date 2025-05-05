#include "NowManager.hpp"

#include <WiFi.h>

#include "Utils.hpp"

bool NowManager::init() {
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error inicializando ESP-NOW");
    return false;
  }

  Serial.println("ESP-NOW inicializado. Esperando datos...");
  return true;
}

bool NowManager::registerSyncPeer() {
  const uint8_t broadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, broadcastMac, 6);
  peerInfo.channel = 0;
  peerInfo.ifidx = WIFI_IF_STA;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Error registrando peer");
    return false;
  }

  Serial.println("Broadcast peer registrado. Enviando datos...");
  return true;
}

void NowManager::setPMK(const String& key) {
  esp_now_set_pmk((uint8_t*)key.c_str());
}

void NowManager::onSend(esp_now_send_cb_t callback) {
  esp_now_register_send_cb(callback);
}

void NowManager::unsuscribeOnSend() { esp_now_unregister_send_cb(); }

void NowManager::onReceived(esp_now_recv_cb_t callback) {
  esp_now_register_recv_cb(callback);
}

void NowManager::unsuscribeOnReceived() { esp_now_unregister_recv_cb(); }

bool NowManager::sendSyncBroadcast() {
  NowManager::SyncBroadcastMsg msg;
  msg.pairingCode = esp_random();

  // Generate CRC8
  addCRC8(msg);

  // Calcular checksum
  // msg.checksum = 0;
  // uint8_t* data = (uint8_t*)&msg;
  // for (size_t i = 0; i < sizeof(NowManager::SyncBroadcast) - 1; i++)
  //   msg.checksum ^= data[i];

  if (esp_now_send(_broadcastMac, (uint8_t*)&msg, sizeof(msg)) == ESP_OK) {
    Serial.print("Mensaje enviado a: ");
    Serial.println(macToString(_broadcastMac));

    return true;
  } else {
    Serial.print("Error enviando mensaje a: ");
    Serial.println(macToString(_broadcastMac));

    return false;
  }
}
