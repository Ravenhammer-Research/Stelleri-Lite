/**
 * @file WlanAuthMode.hpp
 * @brief WLAN authentication mode enumeration and helpers
 */
#pragma once

#include <string>

enum class WlanAuthMode { OPEN = 0, SHARED = 1, WPA = 2, WPA2 = 3, UNKNOWN = 99 };

static inline std::string WlanAuthModeToString(WlanAuthMode m) {
  switch (m) {
  case WlanAuthMode::OPEN:
    return "OPEN";
  case WlanAuthMode::SHARED:
    return "SHARED";
  case WlanAuthMode::WPA:
    return "WPA";
  case WlanAuthMode::WPA2:
    return "WPA2";
  default:
    return "unknown";
  }
}
