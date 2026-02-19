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

#include "WlanTableFormatter.hpp"
#include "ConfigurationManager.hpp"
#include "InterfaceConfig.hpp"
#include "InterfaceFlags.hpp"
#include "WlanAuthMode.hpp"
#include "WlanInterfaceConfig.hpp"

namespace {

// Build a condensed capability string from the three caps bitmasks.
std::string buildCapsString(uint32_t dc, uint32_t ht, uint32_t vht) {
  std::string s;

  // Driver caps (WlanDriverCap)
  if (hasWlanDriverCap(dc, WlanDriverCap::STA))        s += 'S';
  if (hasWlanDriverCap(dc, WlanDriverCap::IBSS))       s += 'I';
  if (hasWlanDriverCap(dc, WlanDriverCap::HOSTAP))     s += 'H';
  if (hasWlanDriverCap(dc, WlanDriverCap::MONITOR))    s += 'M';
  if (hasWlanDriverCap(dc, WlanDriverCap::PMGT))       s += 'P';
  if (hasWlanDriverCap(dc, WlanDriverCap::SHSLOT))     s += 's';
  if (hasWlanDriverCap(dc, WlanDriverCap::SHPREAMBLE)) s += 'p';
  if (hasWlanDriverCap(dc, WlanDriverCap::DFS))        s += 'D';
  if (hasWlanDriverCap(dc, WlanDriverCap::MBSS))       s += 'm';
  if (hasWlanDriverCap(dc, WlanDriverCap::BGSCAN))     s += 'b';
  if (hasWlanDriverCap(dc, WlanDriverCap::BURST))      s += 'B';
  if (hasWlanDriverCap(dc, WlanDriverCap::WME))        s += 'W';
  if (hasWlanDriverCap(dc, WlanDriverCap::WDS))        s += 'w';
  if (hasWlanDriverCap(dc, WlanDriverCap::TXFRAG))     s += 'F';
  if (hasWlanDriverCap(dc, WlanDriverCap::TDMA))       s += 'T';
  if (hasWlanDriverCap(dc, WlanDriverCap::WPA1))       s += '1';
  if (hasWlanDriverCap(dc, WlanDriverCap::WPA2))       s += '2';
  if (hasWlanDriverCap(dc, WlanDriverCap::TXPMGT))     s += 't';
  if (hasWlanDriverCap(dc, WlanDriverCap::SWRETRY))    s += 'r';

  // HT caps (WlanHTCap)
  if (hasWlanHTCap(ht, WlanHTCap::HT))       s += 'h';
  if (hasWlanHTCap(ht, WlanHTCap::AMPDU))    s += 'A';
  if (hasWlanHTCap(ht, WlanHTCap::AMSDU))    s += 'a';
  if (hasWlanHTCap(ht, WlanHTCap::TXLDPC))   s += 'L';
  if (hasWlanHTCap(ht, WlanHTCap::SMPS))     s += 'x';

  // VHT caps
  if (vht)                                   s += 'V';

  return s.empty() ? "-" : s;
}

} // anonymous namespace

std::string
WlanTableFormatter::format(const std::vector<InterfaceConfig> &items) {
  if (items.empty())
    return "No wireless interfaces found.\n";

  // Re-query via the manager to get full WlanInterfaceConfig objects
  // (the input vector is sliced to base InterfaceConfig).
  std::vector<WlanInterfaceConfig> wlanIfaces;
  if (mgr_)
    wlanIfaces = mgr_->GetWlanInterfaces();

  addColumn("Interface", "Interface", 10, 9, true);
  addColumn("SSID", "SSID", 9, 4, true);
  addColumn("Channel", "Chan", 6, 4, true);
  addColumn("Parent", "Parent", 8, 5, true);
  addColumn("Status", "Status", 5, 10, true);
  addColumn("Auth", "Auth", 6, 4, true);
  addColumn("Caps", "HWCaps", 4, 6, true);

  for (const auto &ic : items) {
    if (ic.type != InterfaceType::Wireless)
      continue;

    std::string status = "-";
    std::string ssid = "-";
    std::string channel = "-";
    std::string parent = "-";
    std::string auth = "-";
    std::string caps = "-";

    // Find matching WlanInterfaceConfig from re-queried data
    const WlanInterfaceConfig *w = nullptr;
    for (const auto &wl : wlanIfaces) {
      if (wl.name == ic.name) {
        w = &wl;
        break;
      }
    }

    if (w) {
      if (w->status)
        status = *w->status;
      else if (ic.flags) {
        if (hasFlag(*ic.flags, InterfaceFlag::RUNNING))
          status = "active";
        else if (hasFlag(*ic.flags, InterfaceFlag::UP))
          status = "no-carrier";
        else
          status = "down";
      }
      ssid = w->ssid ? *w->ssid : std::string("-");
      if (w->channel) {
        channel = std::to_string(*w->channel);
        if (w->channel_freq)
          channel += " (" + std::to_string(*w->channel_freq) + " MHz)";
      }
      parent = w->parent ? *w->parent : std::string("-");
      if (w->authmode) {
        int wpa = w->wpa_version ? *w->wpa_version : 0;
        auth = WlanAuthModeToString(*w->authmode, wpa);
      }
      caps = buildCapsString(
          w->drivercaps.value_or(0),
          w->htcaps.value_or(0),
          w->vhtcaps.value_or(0));
    } else {
      if (ic.flags) {
        if (hasFlag(*ic.flags, InterfaceFlag::RUNNING))
          status = "active";
        else if (hasFlag(*ic.flags, InterfaceFlag::UP))
          status = "no-carrier";
        else
          status = "down";
      }
    }

    addRow({ic.name, ssid, channel, parent, status, auth, caps});
  }

  // Legend for HWCaps letters (bold letter = legend key)
  const std::string BLD = "\x1b[1m";
  const std::string RST = "\x1b[0m";
  std::string legend;
  legend += "HWCaps: ";
  // Driver capabilities
  legend += BLD + "S" + RST + "=STA, ";
  legend += BLD + "I" + RST + "=IBSS, ";
  legend += BLD + "H" + RST + "=HOSTAP, ";
  legend += BLD + "M" + RST + "=MONITOR, ";
  legend += BLD + "P" + RST + "=PMGT, ";
  legend += BLD + "s" + RST + "=SHSLOT,";
  legend += "\n        ";
  legend += BLD + "p" + RST + "=SHPREAMBLE, ";
  legend += BLD + "D" + RST + "=DFS, ";
  legend += BLD + "m" + RST + "=MBSS, ";
  legend += BLD + "b" + RST + "=BGSCAN, ";
  legend += BLD + "B" + RST + "=BURST,";
  legend += "\n        ";
  legend += BLD + "W" + RST + "=WME, ";
  legend += BLD + "w" + RST + "=WDS, ";
  legend += BLD + "F" + RST + "=TXFRAG, ";
  legend += BLD + "T" + RST + "=TDMA, ";
  legend += BLD + "t" + RST + "=TXPMGT, ";
  legend += BLD + "r" + RST + "=SWRETRY,";
  legend += "\n        ";
  legend += BLD + "1" + RST + "=WPA1, ";
  legend += BLD + "2" + RST + "=WPA2, ";
  // HT capabilities
  legend += BLD + "h" + RST + "=HT, ";
  legend += BLD + "A" + RST + "=AMPDU, ";
  legend += BLD + "a" + RST + "=AMSDU, ";
  legend += BLD + "L" + RST + "=LDPC,";
  legend += "\n        ";
  legend += BLD + "x" + RST + "=SMPS, ";
  legend += BLD + "V" + RST + "=VHT";
  legend += "\n\n";

  auto out = renderTable(1000);
  out = legend + out;
  return out;
}
