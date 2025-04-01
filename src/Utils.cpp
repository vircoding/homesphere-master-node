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