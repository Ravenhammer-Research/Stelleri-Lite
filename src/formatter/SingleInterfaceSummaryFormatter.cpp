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

#include "SingleInterfaceSummaryFormatter.hpp"
#include "BridgeInterfaceConfig.hpp"
#include "ConfigurationManager.hpp"
#include "InterfaceConfig.hpp"
#include "InterfaceFlags.hpp"
#include "InterfaceType.hpp"
#include "LaggInterfaceConfig.hpp"
#include "TunInterfaceConfig.hpp"
#include "GifInterfaceConfig.hpp"
#include "OvpnInterfaceConfig.hpp"
#include "IpsecInterfaceConfig.hpp"
#include "VlanInterfaceConfig.hpp"
#include "WlanAuthMode.hpp"
#include "WlanInterfaceConfig.hpp"
#include <sstream>

std::string
SingleInterfaceSummaryFormatter::format(const InterfaceConfig &ic) const {
  std::ostringstream oss;

  oss << "Interface: " << ic.name << "\n";
  oss << "Type:      " << interfaceTypeToString(ic.type) << "\n";

  if (ic.description)
    oss << "Descr:     " << *ic.description << "\n";

  if (ic.hwaddr)
    oss << "HWaddr:    " << *ic.hwaddr << "\n";

  if (ic.flags) {
    std::string status = "-";
    if (*ic.flags & static_cast<uint32_t>(InterfaceFlag::RUNNING)) {
      status = "active";
    } else if (*ic.flags & static_cast<uint32_t>(InterfaceFlag::UP)) {
      status = "no-carrier";
    } else {
      status = "down";
    }
    oss << "Status:    " << status << "\n";
  }

  if (ic.mtu) {
    oss << "MTU:       " << *ic.mtu << "\n";
  }

  if (ic.metric && *ic.metric != 0)
    oss << "Metric:    " << *ic.metric << "\n";

  if (ic.baudrate && *ic.baudrate > 0) {
    uint64_t bps = *ic.baudrate;
    if (bps >= 1'000'000'000)
      oss << "Speed:     " << (bps / 1'000'000'000) << " Gbps\n";
    else if (bps >= 1'000'000)
      oss << "Speed:     " << (bps / 1'000'000) << " Mbps\n";
    else if (bps >= 1'000)
      oss << "Speed:     " << (bps / 1'000) << " Kbps\n";
    else
      oss << "Speed:     " << bps << " bps\n";
  }

  if (ic.link_state) {
    const char *ls = "unknown";
    switch (*ic.link_state) {
    case 1:
      ls = "down";
      break;
    case 2:
      ls = "up";
      break;
    }
    oss << "Link:      " << ls << "\n";
  }

  if (ic.capabilities) {
    oss << "HW Caps:   0x" << std::hex << *ic.capabilities << std::dec << "\n";
  }

  if (ic.status_str)
    oss << "Driver:    "
        << *ic.status_str; // status_str typically includes a trailing newline

  if (ic.address) {
    oss << "Address:   " << ic.address->toString() << "\n";
  }

  for (const auto &alias : ic.aliases) {
    oss << "           " << alias->toString() << "\n";
  }

  if (ic.vrf) {
    oss << "VRF:       " << ic.vrf->table << "\n";
  }

  // Tunnel details (support multiple tunnel-derived config types)
  if (auto tptr = dynamic_cast<const TunInterfaceConfig *>(&ic)) {
    if (tptr->tunnel_vrf)
      oss << "Tunnel VRF: " << *tptr->tunnel_vrf << "\n";
    if (tptr->source)
      oss << "Tunnel Src: " << tptr->source->toString() << "\n";
    if (tptr->destination)
      oss << "Tunnel Dst: " << tptr->destination->toString() << "\n";
  }
  if (auto gptr = dynamic_cast<const GifInterfaceConfig *>(&ic)) {
    if (gptr->tunnel_vrf)
      oss << "Tunnel VRF: " << *gptr->tunnel_vrf << "\n";
    if (gptr->source)
      oss << "Tunnel Src: " << gptr->source->toString() << "\n";
    if (gptr->destination)
      oss << "Tunnel Dst: " << gptr->destination->toString() << "\n";
  }
  if (auto optr = dynamic_cast<const OvpnInterfaceConfig *>(&ic)) {
    if (optr->tunnel_vrf)
      oss << "Tunnel VRF: " << *optr->tunnel_vrf << "\n";
    if (optr->source)
      oss << "Tunnel Src: " << optr->source->toString() << "\n";
    if (optr->destination)
      oss << "Tunnel Dst: " << optr->destination->toString() << "\n";
  }
  if (auto iptr = dynamic_cast<const IpsecInterfaceConfig *>(&ic)) {
    if (iptr->tunnel_vrf)
      oss << "Tunnel VRF: " << *iptr->tunnel_vrf << "\n";
    if (iptr->source)
      oss << "Tunnel Src: " << iptr->source->toString() << "\n";
    if (iptr->destination)
      oss << "Tunnel Dst: " << iptr->destination->toString() << "\n";
  }

  // Show bridge details
  if (auto bptr = dynamic_cast<const BridgeInterfaceConfig *>(&ic)) {
    oss << "Bridge STP: " << (bptr->stp ? "enabled" : "disabled") << "\n";
    if (!bptr->members.empty()) {
      oss << "Members:   ";
      bool first = true;
      for (const auto &m : bptr->members) {
        if (!first)
          oss << ", ";
        oss << m;
        first = false;
      }
      oss << "\n";
    }
  }

  // Show LAGG details
  if (auto lptr = dynamic_cast<const LaggInterfaceConfig *>(&ic)) {
    oss << "LAGG Proto: ";
    switch (lptr->protocol) {
    case LaggProtocol::LACP:
      oss << "LACP";
      break;
    case LaggProtocol::FAILOVER:
      oss << "Failover";
      break;
    case LaggProtocol::LOADBALANCE:
      oss << "Load Balance";
      break;
    case LaggProtocol::ROUNDROBIN:
      oss << "Round Robin";
      break;
    case LaggProtocol::BROADCAST:
      oss << "Broadcast";
      break;
    case LaggProtocol::NONE:
      oss << "None";
      break;
    }
    oss << "\n";

    if (!lptr->members.empty()) {
      oss << "Members:   ";
      bool first = true;
      for (const auto &m : lptr->members) {
        if (!first)
          oss << ", ";
        oss << m;
        first = false;
      }
      oss << "\n";
    }
  }

  // Show VLAN details
  if (auto vptr = dynamic_cast<const VlanInterfaceConfig *>(&ic)) {
    oss << "VLAN ID:   " << vptr->id << "\n";
    if (vptr->parent)
      oss << "Parent:    " << *vptr->parent << "\n";
    if (vptr->pcp)
      oss << "PCP:       " << static_cast<int>(*vptr->pcp) << "\n";
  }

  // Show wireless details — re-query via mgr to get WlanInterfaceConfig
  if (ic.type == InterfaceType::Wireless && mgr_) {
    auto wlanIfaces = mgr_->GetWlanInterfaces();
    const WlanInterfaceConfig *w = nullptr;
    for (const auto &wl : wlanIfaces) {
      if (wl.name == ic.name) {
        w = &wl;
        break;
      }
    }
    if (w) {
      if (w->ssid)
        oss << "SSID:      " << *w->ssid << "\n";
      if (w->channel) {
        oss << "Channel:   " << *w->channel;
        if (w->channel_freq)
          oss << " (" << *w->channel_freq << " MHz)";
        oss << "\n";
      }
      if (w->bssid)
        oss << "BSSID:     " << *w->bssid << "\n";
      if (w->regdomain || w->country) {
        oss << "RegDomain: ";
        if (w->regdomain) oss << *w->regdomain;
        if (w->country) oss << " country " << *w->country;
        oss << "\n";
      }
      if (w->parent)
        oss << "Parent:    " << *w->parent << "\n";
      if (w->authmode) {
        int wpa = w->wpa_version ? *w->wpa_version : 0;
        oss << "Auth:      " << WlanAuthModeToString(*w->authmode, wpa) << "\n";
      }
      if (w->privacy)
        oss << "Privacy:   " << (*w->privacy ? "ON" : "OFF") << "\n";
      // deftxkey + cipher
      {
        bool hasDeftx = w->deftxkey.has_value();
        bool hasCipher = w->cipher.has_value();
        if (hasDeftx || hasCipher) {
          oss << "DefTxKey:  ";
          if (hasDeftx)
            oss << (*w->deftxkey == -1 ? "UNDEF" : std::to_string(*w->deftxkey));
          if (hasCipher) {
            oss << " " << WlanCipherToString(*w->cipher);
            if (w->cipher_keylen)
              oss << " " << *w->cipher_keylen << "-bit";
          }
          oss << "\n";
        }
      }
      if (w->txpower)
        oss << "TxPower:   " << *w->txpower << " dBm\n";
      if (w->bmiss)
        oss << "BMiss:     " << *w->bmiss << "\n";
      // mcastrate / mgmtrate
      if (w->mcastrate) {
        int r = *w->mcastrate;
        if (r & 0x80)
          oss << "McastRate: MCS " << (r & 0x7f) << "\n";
        else
          oss << "McastRate: " << (r / 2) << "\n";
      }
      if (w->mgmtrate) {
        int r = *w->mgmtrate;
        if (r & 0x80)
          oss << "MgmtRate:  MCS " << (r & 0x7f) << "\n";
        else
          oss << "MgmtRate:  " << (r / 2) << "\n";
      }
      if (w->maxretry)
        oss << "MaxRetry:  " << *w->maxretry << "\n";
      if (w->scanvalid)
        oss << "ScanValid: " << *w->scanvalid << "\n";
      // HT settings
      if (w->htconf) {
        const char *h = "off";
        switch (*w->htconf & 3) {
        case 1: h = "ht20"; break;
        case 3: h = "ht"; break;
        }
        oss << "HTConf:    " << h << "\n";
      }
      if (w->ampdu) {
        std::string a;
        switch (*w->ampdu) {
        case 0: a = "-ampdu"; break;
        case 1: a = "ampdutx -ampdurx"; break;
        case 2: a = "-ampdutx ampdurx"; break;
        case 3: a = "ampdu"; break;
        default: a = std::to_string(*w->ampdu); break;
        }
        oss << "AMPDU:     " << a;
        if (w->ampdu_limit)
          oss << " limit " << WlanAmpduLimitToString(*w->ampdu_limit);
        oss << "\n";
      }
      if (w->shortgi)
        oss << "ShortGI:   " << (*w->shortgi ? "ON" : "OFF") << "\n";
      if (w->stbc) {
        const char *v = "-stbc";
        switch (*w->stbc) {
        case 1: v = "stbctx -stbcrx"; break;
        case 2: v = "-stbctx stbcrx"; break;
        case 3: v = "stbc"; break;
        }
        oss << "STBC:      " << v << "\n";
      }
      if (w->uapsd)
        oss << "UAPSD:     " << (*w->uapsd ? "ON" : "OFF") << "\n";
      if (w->wme)
        oss << "WME:       " << (*w->wme ? "ON" : "OFF") << "\n";
      if (w->roaming)
        oss << "Roaming:   " << WlanRoamingToString(*w->roaming) << "\n";
      if (w->media_subtype || w->media_mode)
        oss << "Media:     " << WlanMediaToString(w->media_subtype, w->media_mode) << "\n";
      if (w->drivercaps)
        oss << "DrvCaps:   0x" << std::hex << *w->drivercaps << std::dec << "\n";
      if (w->htcaps)
        oss << "HTCaps:    0x" << std::hex << *w->htcaps << std::dec << "\n";
      if (w->vhtcaps)
        oss << "VHTCaps:   0x" << std::hex << *w->vhtcaps << std::dec << "\n";
      if (w->status)
        oss << "WlanStat:  " << *w->status << "\n";
    }
  }

  if (ic.flags) {
    oss << "Flags:     ";
    if (*ic.flags & static_cast<uint32_t>(InterfaceFlag::UP))
      oss << "UP ";
    if (*ic.flags & static_cast<uint32_t>(InterfaceFlag::BROADCAST))
      oss << "BROADCAST ";
    if (*ic.flags & static_cast<uint32_t>(InterfaceFlag::LOOPBACK))
      oss << "LOOPBACK ";
    if (*ic.flags & static_cast<uint32_t>(InterfaceFlag::POINTOPOINT))
      oss << "POINTOPOINT ";
    if (*ic.flags & static_cast<uint32_t>(InterfaceFlag::RUNNING))
      oss << "RUNNING ";
    if (*ic.flags & static_cast<uint32_t>(InterfaceFlag::MULTICAST))
      oss << "MULTICAST";
    oss << "\n";
  }

  return oss.str();
}
