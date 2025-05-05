#pragma once

#include <Arduino.h>
#include <esp_now.h>

#include <algorithm>
#include <vector>

class NowManager {
 public:
  enum class MessageType {
    SYNC_BROADCAST = 0x55,
    REGISTRATION = 0xAA,
    CONFIRM_REGISTRATION = 0xCC
  };

#pragma pack(push, 1)  // Empaquetamiento estricto sin padding
  struct SyncBroadcastMsg {
    uint8_t msgType = static_cast<uint8_t>(
        MessageType::SYNC_BROADCAST);  // Mensaje de sincronizacion
    uint32_t pairingCode;              // Codigo unico generado
    uint8_t crc;                       // XOR de todos los bytes
  };

  struct RegistrationMsg {
    uint8_t msgType =
        static_cast<uint8_t>(MessageType::REGISTRATION);  // Mensaje de registro
    uint8_t nodeType;
    uint8_t firmwareVersion[3];
    uint8_t crc;
  };

  struct ConfirmRegistrationMsg {
    uint8_t msgType = static_cast<uint8_t>(MessageType::CONFIRM_REGISTRATION);
  };
#pragma pack(pop)

  struct DeviceInfo {
    uint8_t mac[6];              // Dirección MAC
    uint8_t nodeType;            // Tipo de nodo
    uint8_t firmwareVersion[3];  // Version del firmware del nodo
    uint32_t lastSeen;           // Timestamp de última comunicación
  };

  struct TemperatureData {
    float hum;
    float temp;
    int node_id;
  };

  bool init();
  bool stop();
  bool reset();
  bool registerPeer(const uint8_t* mac);
  bool registerBroadcastPeer();
  void onSend(esp_now_send_cb_t callback);
  void unsuscribeOnSend();
  void onReceived(esp_now_recv_cb_t callback);
  void unsuscribeOnReceived();
  bool sendSyncBroadcastMsg();
  bool sendConfirmRegistrationMsg(const uint8_t* mac);
  static bool validateMessage(MessageType expectedType, const uint8_t* data,
                              size_t length);

 private:
  static constexpr uint32_t SYNC_MODE_TIMEOUT = 30000;

  uint8_t _broadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  std::vector<DeviceInfo> _pairedDevices;
  bool _isBroadcastPeerRegistered = false;

  static size_t _getMessageSize(MessageType type);
  bool _addDevice(const uint8_t* mac, uint8_t nodeType,
                  uint8_t* firmwareVersion);
};