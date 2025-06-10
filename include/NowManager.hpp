#pragma once

#include <Arduino.h>
#include <esp_now.h>

#include <algorithm>
#include <vector>

class NowManager {
 public:
  static constexpr uint32_t SYNC_MODE_TIMEOUT = 30000;                // 30s
  static constexpr uint32_t SEND_SYNC_BROADCAST_MSG_INTERVAL = 5000;  // 5s
  static constexpr uint32_t PING_ALL_DEVICES_INTERVAL = 10000;        // 10s

  enum class NodeType {
    TEMPERATURE_HUMIDITY = 0x1A,
    RELAY = 0x2B,
  };

  enum class MessageType {
    SYNC_BROADCAST = 0x55,
    REGISTRATION = 0xAA,
    CONFIRM_REGISTRATION = 0xCC,
    TEMPERATURE_HUMIDITY = 0x1A,
    SET_ACTUATOR = 0xB3,
    ACTUATOR_STATE = 0x26,
    SCHEDULE_ACTUATOR = 0x33,
    PING = 0x11
  };

  enum class SensorValueType { FLOAT, INT, BOOL };

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

  struct TemperatureHumidityMsg {
    uint8_t msgType = static_cast<uint8_t>(MessageType::TEMPERATURE_HUMIDITY);
    float temp;
    float hum;
    uint8_t crc;
  };

  struct SetActuatorMsg {
    uint8_t msgType = static_cast<uint8_t>(MessageType::SET_ACTUATOR);
    bool state;
    uint8_t crc;
  };

  struct ActuatorStateMsg {
    uint8_t msgType = static_cast<uint8_t>(MessageType::ACTUATOR_STATE);
    bool state;
    uint8_t crc;
  };

  struct ScheduleActuatorMsg {
    uint8_t msgType = static_cast<uint8_t>(MessageType::SCHEDULE_ACTUATOR);
    uint32_t offset;
    uint32_t duration;
    uint8_t crc;
  };

  struct PingMsg {
    uint8_t msgType = static_cast<uint8_t>(MessageType::PING);
  };
#pragma pack(pop)

  struct DeviceInfo {
    uint8_t mac[6];              // Dirección MAC
    uint8_t nodeType;            // Tipo de nodo
    String deviceName;           // Nombre del nodo
    uint8_t firmwareVersion[3];  // Version del firmware del nodo
    uint8_t nodeId;              // ID asignado para la red
    uint32_t lastSeen;           // Timestamp de última comunicación
  };

  struct SensorData {
    uint8_t mac[6];
    String deviceName;
    bool isConnected;
    String variable;
    String units;
    SensorValueType type;
    union {
      float f;
      int i;
      bool b;
    } value;
  };

  struct ActuatorData {
    uint8_t mac[6];
    String deviceName;
    bool isConnected;
    bool state;
  };

  bool init();
  bool stop();
  bool reset();
  bool registerBroadcastPeer();
  void onSend(esp_now_send_cb_t callback);
  void unsuscribeOnSend();
  void onReceived(esp_now_recv_cb_t callback);
  void unsuscribeOnReceived();
  bool sendSyncBroadcastMsg();
  bool sendConfirmRegistrationMsg(const uint8_t* mac);
  bool sendSetActuatorMsg(const uint8_t* mac, const bool state);
  bool sendScheduleActuatorMsg(const uint8_t* mac, const uint32_t offset = 0,
                               const uint32_t duration = 0xFFFFFFFF);
  bool sendPingMsg(const uint8_t* mac);
  static bool validateMessage(MessageType expectedType, const uint8_t* data,
                              size_t length);
  DeviceInfo* findDevice(const uint8_t* mac);
  bool removeDevice(const uint8_t* mac);
  bool removeSensor(const uint8_t* mac, const String& variable);
  bool removeActuator(const uint8_t* mac);
  bool addDevice(const uint8_t* mac, const uint8_t nodeType,
                 const String& deviceName, const uint8_t* firmwareVersion);
  bool isDevicePaired(const uint8_t* mac);
  void updateDeviceLastSeen(const uint8_t* mac);
  void printAllDevices();
  std::vector<DeviceInfo> getDeviceList() const { return _pairedDevices; }
  std::vector<SensorData> getSensorList() const { return _sensors; }
  std::vector<ActuatorData> getActuatorList() const { return _actuators; }
  size_t getSensorListSize() const { return _sensors.size(); }
  size_t getActuatorListSize() const { return _actuators.size(); }
  SensorData getSensorAt(const int index) const;
  ActuatorData getActuatorAt(const int index) const;
  bool getIsDataTransferEnabled() const { return _isDataTransferEnabled; }
  void setDataTransfer(const bool state);
  void updateSensorData(
      const uint8_t* mac, const String& variable,
      const bool value);  // Metodo sobrecargado para sensores binarios
  void updateSensorData(
      const uint8_t* mac, const String& variable,
      const int value);  // Metodo sobrecargado para sensores discretos
  void updateSensorData(
      const uint8_t* mac, const String& variable,
      const float value);  // Metodo sobrecargado para sensores continuos
  void updateActuatorState(const uint8_t* mac, const bool state);
  void desconnectSensor(const uint8_t* mac, const String& variable);
  void desconnectActuator(const uint8_t* mac);

 private:
  bool _isDataTransferEnabled = false;
  uint8_t _broadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  bool _isBroadcastPeerRegistered = false;
  std::vector<DeviceInfo> _pairedDevices;
  std::vector<SensorData> _sensors;
  std::vector<ActuatorData> _actuators;

  static size_t _getMessageSize(MessageType type);
  bool _registerPeer(const uint8_t* mac);
};