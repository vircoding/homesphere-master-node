#pragma once

#include <WiFi.h>

void sanitizeInput(String& input, size_t maxLength = 32);
bool isNetworkSecure(wifi_auth_mode_t ecryptionType);
String formatBooleanToText(const bool data);