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

#include "SingleWlanSummaryFormatter.hpp"
#include "SingleInterfaceSummaryFormatter.hpp"
#include "WlanAuthMode.hpp"
#include <sstream>

std::string
SingleWlanSummaryFormatter::format(const WlanInterfaceConfig &w) const {
  // Base InterfaceConfig fields
  SingleInterfaceSummaryFormatter base;
  std::string out = base.format(w);

  std::ostringstream oss;

  if (w.ssid)
    oss << "SSID:      " << *w.ssid << "\n";
  if (w.channel) {
    oss << "Channel:   " << *w.channel;
    if (w.channel_freq)
      oss << " (" << *w.channel_freq << " MHz)";
    oss << "\n";
  }
  if (w.bssid)
    oss << "BSSID:     " << *w.bssid << "\n";
  if (w.regdomain || w.country) {
    oss << "RegDomain: ";
    if (w.regdomain)
      oss << *w.regdomain;
    if (w.country)
      oss << " country " << *w.country;
    oss << "\n";
  }
  if (w.parent)
    oss << "Parent:    " << *w.parent << "\n";
  if (w.authmode) {
    int wpa = w.wpa_version ? *w.wpa_version : 0;
    oss << "Auth:      " << WlanAuthModeToString(*w.authmode, wpa) << "\n";
  }
  if (w.privacy)
    oss << "Privacy:   " << (*w.privacy ? "ON" : "OFF") << "\n";
  // deftxkey + cipher
  {
    bool hasDeftx = w.deftxkey.has_value();
    bool hasCipher = w.cipher.has_value();
    if (hasDeftx || hasCipher) {
      oss << "DefTxKey:  ";
      if (hasDeftx)
        oss << (*w.deftxkey == -1 ? "UNDEF" : std::to_string(*w.deftxkey));
      if (hasCipher) {
        oss << " " << WlanCipherToString(*w.cipher);
        if (w.cipher_keylen)
          oss << " " << *w.cipher_keylen << "-bit";
      }
      oss << "\n";
    }
  }
  if (w.txpower)
    oss << "TxPower:   " << *w.txpower << " dBm\n";
  if (w.bmiss)
    oss << "BMiss:     " << *w.bmiss << "\n";
  // mcastrate / mgmtrate
  if (w.mcastrate) {
    int r = *w.mcastrate;
    if (r & 0x80)
      oss << "McastRate: MCS " << (r & 0x7f) << "\n";
    else
      oss << "McastRate: " << (r / 2) << "\n";
  }
  if (w.mgmtrate) {
    int r = *w.mgmtrate;
    if (r & 0x80)
      oss << "MgmtRate:  MCS " << (r & 0x7f) << "\n";
    else
      oss << "MgmtRate:  " << (r / 2) << "\n";
  }
  if (w.maxretry)
    oss << "MaxRetry:  " << *w.maxretry << "\n";
  if (w.scanvalid)
    oss << "ScanValid: " << *w.scanvalid << "\n";
  // HT settings
  if (w.htconf) {
    const char *h = "off";
    switch (*w.htconf & 3) {
    case 1:
      h = "ht20";
      break;
    case 3:
      h = "ht";
      break;
    }
    oss << "HTConf:    " << h << "\n";
  }
  if (w.ampdu) {
    std::string a;
    switch (*w.ampdu) {
    case 0:
      a = "-ampdu";
      break;
    case 1:
      a = "ampdutx -ampdurx";
      break;
    case 2:
      a = "-ampdutx ampdurx";
      break;
    case 3:
      a = "ampdu";
      break;
    default:
      a = std::to_string(*w.ampdu);
      break;
    }
    oss << "AMPDU:     " << a;
    if (w.ampdu_limit)
      oss << " limit " << WlanAmpduLimitToString(*w.ampdu_limit);
    oss << "\n";
  }
  if (w.shortgi)
    oss << "ShortGI:   " << (*w.shortgi ? "ON" : "OFF") << "\n";
  if (w.stbc) {
    const char *v = "-stbc";
    switch (*w.stbc) {
    case 1:
      v = "stbctx -stbcrx";
      break;
    case 2:
      v = "-stbctx stbcrx";
      break;
    case 3:
      v = "stbc";
      break;
    }
    oss << "STBC:      " << v << "\n";
  }
  if (w.uapsd)
    oss << "UAPSD:     " << (*w.uapsd ? "ON" : "OFF") << "\n";
  if (w.wme)
    oss << "WME:       " << (*w.wme ? "ON" : "OFF") << "\n";
  if (w.roaming)
    oss << "Roaming:   " << WlanRoamingToString(*w.roaming) << "\n";
  if (w.media_subtype || w.media_mode)
    oss << "Media:     " << WlanMediaToString(w.media_subtype, w.media_mode)
        << "\n";
  if (w.drivercaps)
    oss << "DrvCaps:   0x" << std::hex << *w.drivercaps << std::dec << "\n";
  if (w.htcaps)
    oss << "HTCaps:    0x" << std::hex << *w.htcaps << std::dec << "\n";
  if (w.vhtcaps)
    oss << "VHTCaps:   0x" << std::hex << *w.vhtcaps << std::dec << "\n";
  if (w.status)
    oss << "WlanStat:  " << *w.status << "\n";

  out += oss.str();
  return out;
}
