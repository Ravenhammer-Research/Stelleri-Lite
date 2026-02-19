/*
 * Copyright (c) 2026, Ravenhammer Research Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file WlanAuthMode.hpp
 * @brief WLAN authentication mode enumeration and helpers
 */
#pragma once

#include <string>

/// Auth mode values match FreeBSD IEEE80211_AUTH_* from <net80211/_ieee80211.h>
enum class WlanAuthMode {
  NONE    = 0,  // IEEE80211_AUTH_NONE
  OPEN    = 1,  // IEEE80211_AUTH_OPEN
  SHARED  = 2,  // IEEE80211_AUTH_SHARED
  X8021   = 3,  // IEEE80211_AUTH_8021X
  AUTO    = 4,  // IEEE80211_AUTH_AUTO
  WPA     = 5,  // IEEE80211_AUTH_WPA (covers WPA, WPA2, WPA1+2)
  UNKNOWN = 99
};

/// Render auth mode + WPA version as a human-readable string.
/// @param m    Auth mode from IEEE80211_IOC_AUTHMODE
/// @param wpa  WPA version from IEEE80211_IOC_WPA (0=none, 1=WPA, 2=WPA2, 3=both)
static inline std::string WlanAuthModeToString(WlanAuthMode m, int wpa = 0) {
  switch (m) {
  case WlanAuthMode::NONE:
    return "NONE";
  case WlanAuthMode::OPEN:
    return "OPEN";
  case WlanAuthMode::SHARED:
    return "SHARED";
  case WlanAuthMode::X8021:
    return "802.1X";
  case WlanAuthMode::AUTO:
    return "AUTO";
  case WlanAuthMode::WPA:
    if (wpa == 2)
      return "WPA2/802.11i";
    if (wpa == 3)
      return "WPA1+WPA2";
    if (wpa == 1)
      return "WPA";
    return "WPA";
  default:
    return "unknown";
  }
}
