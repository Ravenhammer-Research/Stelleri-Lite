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
 * @file WlanConfig.hpp
 * @brief Wireless (802.11) interface configuration
 */

#pragma once

#include "ConfigurationManager.hpp"
#include "InterfaceConfig.hpp"
#include "WlanAuthMode.hpp"

#include <string>

/// IEEE 802.11 media sub-type (IFM_TMASK bits)
enum class WlanMediaSubtype {
  AUTO, ///< autoselect
  MCS,  ///< HT MCS rate
  VHT,  ///< VHT MCS rate
};

/// IEEE 802.11 PHY mode (IFM_MMASK / mode bits)
enum class WlanMediaMode {
  AUTO,    ///< autoselect / unknown
  A_11A,   ///< 5 GHz OFDM
  B_11B,   ///< 2.4 GHz DSSS
  G_11G,   ///< 2.4 GHz CCK/OFDM
  NA_11NA, ///< 5 GHz HT
  NG_11NG, ///< 2.4 GHz HT
  VHT_5G,  ///< 5 GHz VHT (11ac)
  VHT_2G,  ///< 2.4 GHz VHT
};

/// IEEE 802.11 roaming policy
enum class WlanRoaming {
  DEVICE = 0,
  AUTO = 1,
  MANUAL = 2,
};

/// IEEE 802.11 driver capability flags (matches IEEE80211_C_* from
/// _ieee80211.h)
enum class WlanDriverCap : uint32_t {
  STA = 0x00000001,
  ENCAP8023 = 0x00000002,
  FF = 0x00000040,
  TURBOP = 0x00000080,
  IBSS = 0x00000100,
  PMGT = 0x00000200,
  HOSTAP = 0x00000400,
  AHDEMO = 0x00000800,
  SWRETRY = 0x00001000,
  TXPMGT = 0x00002000,
  SHSLOT = 0x00004000,
  SHPREAMBLE = 0x00008000,
  MONITOR = 0x00010000,
  DFS = 0x00020000,
  MBSS = 0x00040000,
  SWSLEEP = 0x00080000,
  SWAMSDUTX = 0x00100000,
  UAPSD = 0x00200000,
  WPA1 = 0x00800000,
  WPA2 = 0x01000000,
  BURST = 0x02000000,
  WME = 0x04000000,
  WDS = 0x08000000,
  BGSCAN = 0x20000000,
  TXFRAG = 0x40000000,
  TDMA = 0x80000000,
};

constexpr bool hasWlanDriverCap(uint32_t caps, WlanDriverCap c) noexcept {
  return (caps & static_cast<uint32_t>(c)) != 0;
}

/// IEEE 802.11n HT capability flags (matches IEEE80211_HTC_* from _ieee80211.h)
enum class WlanHTCap : uint32_t {
  AMPDU = 0x00010000,
  AMSDU = 0x00020000,
  HT = 0x00040000,
  SMPS = 0x00080000,
  RIFS = 0x00100000,
  RXUNEQUAL = 0x00200000,
  RXMCS32 = 0x00400000,
  TXUNEQUAL = 0x00800000,
  TXMCS32 = 0x01000000,
  TXLDPC = 0x02000000,
  RX_AMSDU_AMPDU = 0x04000000,
  TX_AMSDU_AMPDU = 0x08000000,
};

constexpr bool hasWlanHTCap(uint32_t caps, WlanHTCap c) noexcept {
  return (caps & static_cast<uint32_t>(c)) != 0;
}

/// IEEE 802.11 cipher type (matches IEEE80211_CIPHER_* from ieee80211_crypto.h)
enum class WlanCipher {
  WEP = 0,
  TKIP = 1,
  AES_OCB = 2,
  AES_CCM = 3,
  TKIPMIC = 4,
  CKIP = 5,
  NONE = 6,
  AES_CCM_256 = 7,
  BIP_CMAC_128 = 8,
  BIP_CMAC_256 = 9,
  BIP_GMAC_128 = 10,
  BIP_GMAC_256 = 11,
  AES_GCM_128 = 12,
  AES_GCM_256 = 13,
};

inline const char *WlanCipherToString(WlanCipher c) {
  switch (c) {
  case WlanCipher::WEP:
    return "WEP";
  case WlanCipher::TKIP:
    return "TKIP";
  case WlanCipher::AES_OCB:
    return "AES-OCB";
  case WlanCipher::AES_CCM:
    return "AES-CCM";
  case WlanCipher::TKIPMIC:
    return "TKIP-MIC";
  case WlanCipher::CKIP:
    return "CKIP";
  case WlanCipher::NONE:
    return "NONE";
  case WlanCipher::AES_CCM_256:
    return "AES-CCM-256";
  case WlanCipher::BIP_CMAC_128:
    return "BIP-CMAC-128";
  case WlanCipher::BIP_CMAC_256:
    return "BIP-CMAC-256";
  case WlanCipher::BIP_GMAC_128:
    return "BIP-GMAC-128";
  case WlanCipher::BIP_GMAC_256:
    return "BIP-GMAC-256";
  case WlanCipher::AES_GCM_128:
    return "AES-GCM-128";
  case WlanCipher::AES_GCM_256:
    return "AES-GCM-256";
  default:
    return "UNKNOWN";
  }
}

/// Convert AMPDU limit code (0-3) to a human-readable string.
inline const char *WlanAmpduLimitToString(int v) {
  switch (v) {
  case 0:
    return "8k";
  case 1:
    return "16k";
  case 2:
    return "32k";
  case 3:
    return "64k";
  default:
    return "?";
  }
}

inline const char *WlanMediaSubtypeToString(WlanMediaSubtype s) {
  switch (s) {
  case WlanMediaSubtype::MCS:
    return "MCS";
  case WlanMediaSubtype::VHT:
    return "VHT";
  default:
    return "auto";
  }
}

inline const char *WlanMediaModeToString(WlanMediaMode m) {
  switch (m) {
  case WlanMediaMode::A_11A:
    return "11a";
  case WlanMediaMode::B_11B:
    return "11b";
  case WlanMediaMode::G_11G:
    return "11g";
  case WlanMediaMode::NA_11NA:
    return "11na";
  case WlanMediaMode::NG_11NG:
    return "11ng";
  case WlanMediaMode::VHT_5G:
    return "11ac";
  case WlanMediaMode::VHT_2G:
    return "11ac2";
  default:
    return "auto";
  }
}

inline const char *WlanRoamingToString(WlanRoaming r) {
  switch (r) {
  case WlanRoaming::DEVICE:
    return "DEVICE";
  case WlanRoaming::AUTO:
    return "AUTO";
  case WlanRoaming::MANUAL:
    return "MANUAL";
  default:
    return "DEVICE";
  }
}

inline std::string WlanMediaToString(std::optional<WlanMediaSubtype> sub,
                                     std::optional<WlanMediaMode> mode) {
  std::string desc = "IEEE 802.11";
  if (sub)
    desc += std::string(" ") + WlanMediaSubtypeToString(*sub);
  if (mode && *mode != WlanMediaMode::AUTO)
    desc += std::string(" mode ") + WlanMediaModeToString(*mode);
  return desc;
}

class WlanInterfaceConfig : public InterfaceConfig {
public:
  explicit WlanInterfaceConfig(const InterfaceConfig &base)
      : InterfaceConfig(base) {}
  void save(ConfigurationManager &mgr) const override;
  void create(ConfigurationManager &mgr) const;
  void destroy(ConfigurationManager &mgr) const override;
  // Wireless-specific attributes
  std::optional<std::string> ssid;
  std::optional<int> channel;
  std::optional<int> channel_freq; ///< Channel centre frequency in MHz
  std::optional<std::string> bssid;
  std::optional<std::string> parent;
  std::optional<WlanAuthMode> authmode;
  std::optional<WlanMediaSubtype>
      media_subtype;                       ///< IFM sub-type (auto/MCS/VHT)
  std::optional<WlanMediaMode> media_mode; ///< PHY mode (11a/b/g/na/ng/ac)
  std::optional<std::string> status;
  std::optional<int> opmode; ///< Operating mode (IEEE80211_M_STA, etc.)
  std::optional<std::string> macaddr; ///< Locally-assigned MAC address
  std::optional<int>
      wpa_version; ///< WPA version (0=none, 1=WPA, 2=WPA2/RSN, 3=both)
  std::optional<int> txpower;         ///< Transmit power (dBm)
  std::optional<bool> privacy;        ///< Privacy (encryption) enabled
  std::optional<WlanRoaming> roaming; ///< Roaming policy
  std::optional<uint32_t> drivercaps; ///< IEEE80211_C_* driver capability bits
  std::optional<uint32_t> htcaps;     ///< HT (802.11n) capability bits
  std::optional<uint32_t> vhtcaps;    ///< VHT (802.11ac) capability bits
  // Regulatory
  std::optional<std::string>
      regdomain; ///< Regulatory domain SKU name (e.g. "FCC")
  std::optional<std::string> country; ///< Country code (e.g. "US")
  // Key / cipher
  std::optional<int> deftxkey; ///< Default TX key index (1-based, -1=UNDEF)
  std::optional<WlanCipher> cipher; ///< Active cipher type
  std::optional<int> cipher_keylen; ///< Cipher key length in bits
  // Operational parameters
  std::optional<int> bmiss;     ///< Beacon miss threshold
  std::optional<int> scanvalid; ///< Scan cache valid threshold (sec)
  std::optional<int>
      mcastrate; ///< Multicast rate (raw; /2 for Mbps, |0x80 = MCS)
  std::optional<int> mgmtrate; ///< Management frame rate
  std::optional<int> maxretry; ///< Max unicast retry count
  // HT / VHT operational
  std::optional<int> htconf;        ///< HT config (0=off, 1=HT20, 3=HT20+40)
  std::optional<int> ampdu;         ///< A-MPDU (0=off, 1=tx, 2=rx, 3=both)
  std::optional<int> ampdu_limit;   ///< A-MPDU length limit (0=8k..3=64k)
  std::optional<int> ampdu_density; ///< A-MPDU density (0-7)
  std::optional<int> amsdu;         ///< A-MSDU (0=off, 1=tx, 2=rx, 3=both)
  std::optional<bool> shortgi;      ///< Short guard interval
  std::optional<int> stbc;          ///< STBC (0=off, 1=tx, 2=rx, 3=both)
  std::optional<int> ldpc;          ///< LDPC (0=off, 1=tx, 2=rx, 3=both)
  std::optional<bool> uapsd;        ///< U-APSD
  std::optional<bool> wme;          ///< WME (Wi-Fi Multimedia)
  // Copy constructor
  WlanInterfaceConfig(const WlanInterfaceConfig &o) : InterfaceConfig(o) {
    ssid = o.ssid;
    channel = o.channel;
    channel_freq = o.channel_freq;
    bssid = o.bssid;
    parent = o.parent;
    authmode = o.authmode;
    media_subtype = o.media_subtype;
    media_mode = o.media_mode;
    status = o.status;
    opmode = o.opmode;
    macaddr = o.macaddr;
    wpa_version = o.wpa_version;
    txpower = o.txpower;
    privacy = o.privacy;
    roaming = o.roaming;
    drivercaps = o.drivercaps;
    htcaps = o.htcaps;
    vhtcaps = o.vhtcaps;
    regdomain = o.regdomain;
    country = o.country;
    deftxkey = o.deftxkey;
    cipher = o.cipher;
    cipher_keylen = o.cipher_keylen;
    bmiss = o.bmiss;
    scanvalid = o.scanvalid;
    mcastrate = o.mcastrate;
    mgmtrate = o.mgmtrate;
    maxretry = o.maxretry;
    htconf = o.htconf;
    ampdu = o.ampdu;
    ampdu_limit = o.ampdu_limit;
    ampdu_density = o.ampdu_density;
    amsdu = o.amsdu;
    shortgi = o.shortgi;
    stbc = o.stbc;
    ldpc = o.ldpc;
    uapsd = o.uapsd;
    wme = o.wme;
  }
  WlanInterfaceConfig &operator=(const WlanInterfaceConfig &o) = delete;
};

// implementations in src/cfg/WlanInterfaceConfig.cpp
