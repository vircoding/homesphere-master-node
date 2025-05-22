#include "Utils.hpp"

void sanitizeInput(String& input, size_t maxLength) {
  input.replace("\\", "");
  input.replace("\"", "");
  input.replace("'", "");
  input.replace(";", "");
  if (input.length() > maxLength) input = input.substring(0, maxLength);
}

bool isNetworkSecure(wifi_auth_mode_t encryptionType) {
  return encryptionType != WIFI_AUTH_OPEN;
}

String formatBooleanToText(const bool data) {
  if (data) return "Si";
  return "No";
}

String macToString(const uint8_t* mac) {
  char buffer[18];
  snprintf(buffer, sizeof(buffer), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0],
           mac[1], mac[2], mac[3], mac[4], mac[5]);

  return String(buffer);
}

void stringToMac(const String& macStr, uint8_t* macDest) {
  if (macStr.length() != 17) return;

  sscanf(macStr.c_str(), "%2hhX:%2hhX:%2hhX:%2hhX:%2hhX:%2hhX", &macDest[0],
         &macDest[1], &macDest[2], &macDest[3], &macDest[4], &macDest[5]);
}

String firmwareVersionToString(const uint8_t* firmwareVersion) {
  char firmwareVersionStr[6];
  snprintf(firmwareVersionStr, sizeof(firmwareVersionStr), "%d.%d.%d",
           firmwareVersion[0], firmwareVersion[1], firmwareVersion[2]);

  return String(firmwareVersionStr);
}

bool stringToFirmwareVersion(const String& firmwareVersionStr,
                             uint8_t* firmwareVersionDest) {
  // Validar formato basico y caracteres
  int dotCount = 0;
  for (unsigned int i = 0; i < firmwareVersionStr.length(); i++) {
    char c = firmwareVersionStr.charAt(i);

    if (c == '.')
      dotCount++;
    else {
      if (!isdigit(c)) return false;
    }
  }

  if (dotCount != 2 || firmwareVersionStr.length() < 5)
    return false;  // Minimo 0.0.0

  // Extraer partes
  int parts[3];
  int scanned = sscanf(firmwareVersionStr.c_str(), "%d.%d.%d", &parts[0],
                       &parts[1], &parts[2]);

  if (scanned != 3) return false;

  // Validar rangos (0-255)
  for (unsigned int i = 0; i < 3; i++) {
    if (parts[i] < 0 || parts[i] > 255) return false;

    firmwareVersionDest[i] = static_cast<uint8_t>(parts[i]);
  }

  return true;
}

uint8_t calcCRC8(const uint8_t* data, size_t length) {
  CRC8 crc(CRC8_DALLAS_MAXIM_POLYNOME);
  crc.reset();
  crc.add(data, length);

  return crc.calc();
}
