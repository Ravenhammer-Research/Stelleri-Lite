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

#include "InterfaceToken.hpp"
#include "BridgeTableFormatter.hpp"
#include "CarpInterfaceConfig.hpp"
#include "GreInterfaceConfig.hpp"
#include "InterfaceConfig.hpp"
#include "InterfaceFlags.hpp"
#include "InterfaceTableFormatter.hpp"
#include "InterfaceType.hpp"
#include "LaggInterfaceConfig.hpp"
#include "LaggTableFormatter.hpp"
#include "SingleInterfaceSummaryFormatter.hpp"
#include "SixToFourInterfaceConfig.hpp"
#include "TapInterfaceConfig.hpp"
#include "TunInterfaceConfig.hpp"
#include "GifInterfaceConfig.hpp"
#include "OvpnInterfaceConfig.hpp"
#include "IpsecInterfaceConfig.hpp"
#include "TunTableFormatter.hpp"
#include "GifTableFormatter.hpp"
#include "OvpnTableFormatter.hpp"
#include "IpsecTableFormatter.hpp"
#include "VlanInterfaceConfig.hpp"
#include "VlanTableFormatter.hpp"
#include "VxlanInterfaceConfig.hpp"
#include "EpairTableFormatter.hpp"
#include "WlanInterfaceConfig.hpp"
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <unordered_set>

InterfaceToken::InterfaceToken(InterfaceType t, std::string name)
    : type_(t), name_(std::move(name)) {}

// toString(ConfigData*) removed — implementation deleted per request

// ---------------------------------------------------------------------------
// Helpers: keyword sets for context-aware autocompletion
// ---------------------------------------------------------------------------
namespace {

/// Keywords applicable to every interface type.
std::vector<std::string> generalKeywords() {
  return {"inet",    "inet6",  "address",     "mtu",  "vrf",
          "group",   "up",     "down",        "status",
          "description"};
}

/// Additional keywords relevant to a specific interface type.
std::vector<std::string> keywordsForType(InterfaceType t) {
  switch (t) {
  case InterfaceType::Bridge:
    return {"member", "stp", "priority"};
  case InterfaceType::VLAN:
    return {"vid", "parent", "vlan"};
  case InterfaceType::Lagg:
    return {"lagg", "members", "protocol"};
  case InterfaceType::Tunnel:
  case InterfaceType::Tun:
  case InterfaceType::Gif:
  case InterfaceType::IPsec:
    return {"source", "destination", "tunnel-vrf"};
  case InterfaceType::GRE:
    return {"source", "destination", "key"};
  case InterfaceType::VXLAN:
    return {"vni", "local", "remote", "port"};
  case InterfaceType::Wireless:
    return {"ssid", "channel", "parent", "authmode"};
  case InterfaceType::Virtual:
    return {};
  default:
    return {};
  }
}

/// Value suggestions for a keyword that expects a fixed set of values.
std::vector<std::string> valuesForKeyword(const std::string &kw) {
  if (kw == "type") {
    return {"ethernet", "loopback", "bridge", "lagg", "vlan",  "tunnel",
            "tun",      "gif",      "gre",    "vxlan", "ipsec", "epair",
            "virtual",  "wireless", "tap",    "ppp"};
  }
  if (kw == "protocol") {
    return {"lacp", "failover", "loadbalance", "roundrobin", "broadcast",
            "none"};
  }
  if (kw == "authmode") {
    return {"open", "shared", "wpa", "wpa2"};
  }
  if (kw == "stp") {
    return {"on", "off"};
  }
  if (kw == "status") {
    return {"up", "down"};
  }
  return {};
}

/// Filter a candidate list by prefix match.
std::vector<std::string> filterPrefix(const std::vector<std::string> &candidates,
                                      std::string_view partial) {
  std::vector<std::string> matches;
  for (const auto &c : candidates) {
    if (c.rfind(partial, 0) == 0)
      matches.push_back(c);
  }
  return matches;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// autoComplete (stateless stub — satisfies Token pure virtual)
// The real logic lives in the context-aware overload below.
// ---------------------------------------------------------------------------
std::vector<std::string>
InterfaceToken::autoComplete(std::string_view /*partial*/) const {
  return {};
}

std::unique_ptr<Token> InterfaceToken::clone() const {
  return std::make_unique<InterfaceToken>(*this);
}

// ---------------------------------------------------------------------------
// autoComplete (context-aware — with manager and token history)
// ---------------------------------------------------------------------------
std::vector<std::string>
InterfaceToken::autoComplete(const std::vector<std::string> &tokens,
                             std::string_view partial,
                             ConfigurationManager *mgr) const {
  std::vector<std::string> matches;

  // If user typed `type` and is now completing the type value, suggest types
  if (expect_type_value_)
    return filterPrefix(valuesForKeyword("type"), partial);

  const std::string prev =
      tokens.empty() ? std::string() : tokens.back();

  // --- Value completions for keywords that expect a fixed set of values ---

  // After "type", suggest type names
  if (prev == "type") {
    return filterPrefix(valuesForKeyword("type"), partial);
  }

  // After "protocol", suggest lagg protocol values
  if (prev == "protocol") {
    return filterPrefix(valuesForKeyword("protocol"), partial);
  }

  // After "authmode", suggest WLAN auth modes
  if (prev == "authmode") {
    return filterPrefix(valuesForKeyword("authmode"), partial);
  }

  // After "stp", suggest on/off
  if (prev == "stp") {
    return filterPrefix(valuesForKeyword("stp"), partial);
  }

  // After "status", suggest up/down
  if (prev == "status") {
    return filterPrefix(valuesForKeyword("status"), partial);
  }

  // --- Value completions from live system state (require mgr) ---

  if (mgr) {
    // After "group", list all known interface groups
    if (prev == "group") {
      std::unordered_set<std::string> groups;
      auto ifs = mgr->GetInterfaces();
      for (const auto &i : ifs) {
        for (const auto &g : i.groups) {
          groups.insert(g);
        }
      }
      for (const auto &g : groups) {
        if (g.rfind(partial, 0) == 0)
          matches.push_back(g);
      }
      return matches;
    }

    // After "name", list all known interface names
    if (prev == "name") {
      auto ifs = mgr->GetInterfaces();
      for (const auto &i : ifs) {
        const std::string &iname = i.name;
        if (iname.rfind(partial, 0) == 0)
          matches.push_back(iname);
      }
      return matches;
    }

    // After "member" or "parent", suggest available interface names
    if (prev == "member" || prev == "parent") {
      auto ifs = mgr->GetInterfaces();
      for (const auto &i : ifs) {
        // Don't suggest the interface being configured as its own member/parent
        if (i.name == name_)
          continue;
        if (i.name.rfind(partial, 0) == 0)
          matches.push_back(i.name);
      }
      return matches;
    }

    // After "members" (lagg), suggest comma-separated interface names
    if (prev == "members") {
      auto ifs = mgr->GetInterfaces();
      // If partial already contains a comma, complete the last segment
      std::string prefix_part;
      std::string last_seg(partial);
      auto comma_pos = last_seg.rfind(',');
      if (comma_pos != std::string::npos) {
        prefix_part = last_seg.substr(0, comma_pos + 1);
        last_seg = last_seg.substr(comma_pos + 1);
      }
      for (const auto &i : ifs) {
        if (i.name == name_)
          continue;
        if (i.name.rfind(last_seg, 0) == 0)
          matches.push_back(prefix_part + i.name);
      }
      return matches;
    }
  } // mgr

  // If the previous token is a known interface name (i.e. we are past
  // "interface name <ifname>"), suggest type-specific and general keywords.
  if (tokens.size() >= 2 && tokens[tokens.size() - 2] == "name") {
    auto opts = generalKeywords();
    opts.push_back("type");
    opts.push_back("group");
    // If we can detect the type from the name, add type-specific keywords
    if (type_ != InterfaceType::Unknown) {
      auto extra = keywordsForType(type_);
      opts.insert(opts.end(), extra.begin(), extra.end());
    }
    return filterPrefix(opts, partial);
  }

  // If a type has been set (from earlier tokens), offer type-aware keywords
  if (type_ != InterfaceType::Unknown) {
    auto opts = generalKeywords();
    auto extra = keywordsForType(type_);
    opts.insert(opts.end(), extra.begin(), extra.end());
    return filterPrefix(opts, partial);
  }

  // Top-level: no name, no type, nothing set yet → primary keywords
  if (name_.empty() && type_ == InterfaceType::Unknown && !vrf && !mtu &&
      !status && !vlan && !lagg && !bridge && !tun && !gif && !ovpn &&
      !ipsec && !vxlan && !wlan && !gre && !carp) {
    return filterPrefix({"name", "group", "type"}, partial);
  }

  // Name set but no attributes yet → narrow choices
  if (!name_.empty() && type_ == InterfaceType::Unknown) {
    auto opts = generalKeywords();
    opts.push_back("type");
    opts.push_back("group");
    return filterPrefix(opts, partial);
  }

  // Fallback: only suggest structural keywords that help narrow context.
  // Type-specific keywords are never offered until a type is known.
  return filterPrefix({"name", "type", "group"}, partial);
}

// Static renderers for interface configs
std::string InterfaceToken::toString(InterfaceConfig *cfg) {
  if (!cfg)
    return std::string();
  std::string result = "interface name " + cfg->name;
  switch (cfg->type) {
  case InterfaceType::VLAN:
    result += " type vlan";
    break;
  case InterfaceType::Lagg:
    result += " type lagg";
    break;
  case InterfaceType::Bridge:
    result += " type bridge";
    break;
  case InterfaceType::Tunnel:
    result += " type tunnel";
    break;
  case InterfaceType::Virtual:
    result += " type epair";
    break;
  case InterfaceType::Loopback:
    result += " type loopback";
    break;
  default:
    break;
  }
  if (cfg->vrf)
    result += " vrf " + std::to_string(cfg->vrf->table);
  if (cfg->mtu)
    result += " mtu " + std::to_string(*cfg->mtu);
  if (!cfg->groups.empty()) {
    for (const auto &g : cfg->groups)
      result += " group " + g;
  }
  return result;
}

std::string InterfaceToken::toString(BridgeInterfaceConfig *cfg) {
  if (!cfg)
    return std::string();
  return std::string("interface type bridge");
}

std::string InterfaceToken::toString(CarpInterfaceConfig *cfg) {
  if (!cfg)
    return std::string();
  std::string s = InterfaceToken::toString(static_cast<InterfaceConfig *>(cfg));
  if (cfg->vhid)
    s += " vhid " + std::to_string(*cfg->vhid);
  if (cfg->advskew)
    s += " advskew " + std::to_string(*cfg->advskew);
  if (cfg->advbase)
    s += " advbase " + std::to_string(*cfg->advbase);
  if (cfg->state)
    s += " state " + *cfg->state;
  return s;
}

std::string InterfaceToken::toString(GreInterfaceConfig *cfg) {
  if (!cfg)
    return std::string();
  std::string s = InterfaceToken::toString(static_cast<InterfaceConfig *>(cfg));
  if (cfg->greSource)
    s += " source " + *cfg->greSource;
  if (cfg->greDestination)
    s += " destination " + *cfg->greDestination;
  return s;
}

std::string InterfaceToken::toString(LaggInterfaceConfig *cfg) {
  if (!cfg)
    return std::string();
  std::string s = InterfaceToken::toString(static_cast<InterfaceConfig *>(cfg));
  for (const auto &m : cfg->members)
    s += " member " + m;
  return s;
}

std::string InterfaceToken::toString(SixToFourInterfaceConfig *cfg) {
  if (!cfg)
    return std::string();
  return InterfaceToken::toString(static_cast<InterfaceConfig *>(cfg));
}

std::string InterfaceToken::toString(TapInterfaceConfig *cfg) {
  if (!cfg)
    return std::string();
  return InterfaceToken::toString(static_cast<InterfaceConfig *>(cfg));
}

// legacy TunnelConfig removed

std::string InterfaceToken::toString(TunInterfaceConfig *cfg) {
  if (!cfg)
    return std::string();
  std::string s = InterfaceToken::toString(static_cast<InterfaceConfig *>(cfg));
  if (cfg->source)
    s += " source " + cfg->source->toString();
  if (cfg->destination)
    s += " destination " + cfg->destination->toString();
  return s;
}

std::string InterfaceToken::toString(GifInterfaceConfig *cfg) {
  if (!cfg)
    return std::string();
  std::string s = InterfaceToken::toString(static_cast<InterfaceConfig *>(cfg));
  if (cfg->source)
    s += " source " + cfg->source->toString();
  if (cfg->destination)
    s += " destination " + cfg->destination->toString();
  return s;
}

std::string InterfaceToken::toString(OvpnInterfaceConfig *cfg) {
  if (!cfg)
    return std::string();
  std::string s = InterfaceToken::toString(static_cast<InterfaceConfig *>(cfg));
  if (cfg->source)
    s += " source " + cfg->source->toString();
  if (cfg->destination)
    s += " destination " + cfg->destination->toString();
  return s;
}

std::string InterfaceToken::toString(IpsecInterfaceConfig *cfg) {
  if (!cfg)
    return std::string();
  std::string s = InterfaceToken::toString(static_cast<InterfaceConfig *>(cfg));
  if (cfg->source)
    s += " source " + cfg->source->toString();
  if (cfg->destination)
    s += " destination " + cfg->destination->toString();
  return s;
}

std::string InterfaceToken::toString(VlanInterfaceConfig *cfg) {
  if (!cfg)
    return std::string();
  std::string s = InterfaceToken::toString(static_cast<InterfaceConfig *>(cfg));
  s += " vid " + std::to_string(cfg->id);
  if (cfg->parent)
    s += " parent " + *cfg->parent;
  return s;
}

std::string InterfaceToken::toString(VxlanInterfaceConfig *cfg) {
  if (!cfg)
    return std::string();
  std::string s = InterfaceToken::toString(static_cast<InterfaceConfig *>(cfg));
  if (cfg->vni)
    s += " vni " + std::to_string(*cfg->vni);
  if (cfg->localAddr)
    s += " local " + *cfg->localAddr;
  if (cfg->remoteAddr)
    s += " remote " + *cfg->remoteAddr;
  return s;
}

std::string InterfaceToken::toString(WlanInterfaceConfig *cfg) {
  if (!cfg)
    return std::string();
  std::string s = InterfaceToken::toString(static_cast<InterfaceConfig *>(cfg));
  if (cfg->ssid)
    s += " ssid " + *cfg->ssid;
  if (cfg->channel)
    s += " channel " + std::to_string(*cfg->channel);
  if (cfg->parent)
    s += " parent " + *cfg->parent;
  return s;
}

void InterfaceToken::parseKeywords(std::shared_ptr<InterfaceToken> &tok,
                                   const std::vector<std::string> &tokens,
                                   size_t &cur) {
  while (cur < tokens.size()) {
    const std::string &kw = tokens[cur];
    if (kw == "inet" || kw == "inet6") {
      tok->address_family = (kw == "inet") ? AF_INET : AF_INET6;
      if (cur + 1 < tokens.size() && tokens[cur + 1] == "address" &&
          cur + 2 < tokens.size()) {
        tok->address = tokens[cur + 2];
        cur += 3;
        continue;
      }
      ++cur;
      continue;
    }
    if (kw == "group" && cur + 1 < tokens.size()) {
      tok->group = tokens[cur + 1];
      cur += 2;
      continue;
    }
    if (kw == "mtu" && cur + 1 < tokens.size()) {
      tok->mtu = std::stoi(tokens[cur + 1]);
      cur += 2;
      continue;
    }
    if ((kw == "fib" || kw == "vrf") && cur + 1 < tokens.size()) {
      tok->vrf = std::stoi(tokens[cur + 1]);
      cur += 2;
      continue;
    }
    if (kw == "vlan") {
      ++cur;
      std::optional<uint16_t> vid;
      std::optional<std::string> parent;
      while (cur < tokens.size()) {
        const std::string &k2 = tokens[cur];
        if (k2 == "id" && cur + 1 < tokens.size()) {
          vid = static_cast<uint16_t>(std::stoi(tokens[cur + 1]));
          cur += 2;
          continue;
        }
        if (k2 == "parent" && cur + 1 < tokens.size()) {
          parent = tokens[cur + 1];
          cur += 2;
          continue;
        }
        break;
      }
      if (vid && parent) {
        tok->vlan.emplace();
        tok->vlan->name = tok->name();
        tok->vlan->id = *vid;
        tok->vlan->parent = *parent;
      }
      continue;
    }
    if (kw == "lagg" || kw == "lag") {
      ++cur;
      LaggInterfaceConfig lc;
      while (cur < tokens.size()) {
        const std::string &k2 = tokens[cur];
        if (k2 == "members" && cur + 1 < tokens.size()) {
          const std::string &m = tokens[cur + 1];
          size_t p = 0, len = m.size();
          while (p < len) {
            size_t q = m.find(',', p);
            if (q == std::string::npos)
              q = len;
            lc.members.emplace_back(m.substr(p, q - p));
            p = q + 1;
          }
          cur += 2;
          continue;
        }
        if (k2 == "protocol" && cur + 1 < tokens.size()) {
          const std::string &proto = tokens[cur + 1];
          if (proto == "lacp")
            lc.protocol = LaggProtocol::LACP;
          else if (proto == "failover")
            lc.protocol = LaggProtocol::FAILOVER;
          else if (proto == "loadbalance")
            lc.protocol = LaggProtocol::LOADBALANCE;
          else if (proto == "roundrobin")
            lc.protocol = LaggProtocol::ROUNDROBIN;
          else if (proto == "broadcast")
            lc.protocol = LaggProtocol::BROADCAST;
          else if (proto == "none")
            lc.protocol = LaggProtocol::NONE;
          cur += 2;
          continue;
        }
        break;
      }
      if (!lc.members.empty()) {
        tok->lagg.emplace();
        tok->lagg->members = std::move(lc.members);
        tok->lagg->protocol = lc.protocol;
        tok->lagg->hash_policy = lc.hash_policy;
        tok->lagg->lacp_rate = lc.lacp_rate;
        tok->lagg->min_links = lc.min_links;
      }
      continue;
    }

    // --- Status keywords ---
    if (kw == "up") {
      tok->status = true;
      ++cur;
      continue;
    }
    if (kw == "down") {
      tok->status = false;
      ++cur;
      continue;
    }
    if (kw == "status" && cur + 1 < tokens.size()) {
      const std::string &val = tokens[cur + 1];
      if (val == "up")
        tok->status = true;
      else if (val == "down")
        tok->status = false;
      cur += 2;
      continue;
    }

    // --- Description ---
    if (kw == "description" && cur + 1 < tokens.size()) {
      tok->description = tokens[cur + 1];
      cur += 2;
      continue;
    }

    // --- Tunnel VRF ---
    if ((kw == "tunnel-vrf" || kw == "tunnel-fib") &&
        cur + 1 < tokens.size()) {
      tok->tunnel_vrf = std::stoi(tokens[cur + 1]);
      cur += 2;
      continue;
    }

    // --- Tunnel source / destination ---
    if (kw == "source" && cur + 1 < tokens.size()) {
      tok->source = tokens[cur + 1];
      cur += 2;
      continue;
    }
    if (kw == "destination" && cur + 1 < tokens.size()) {
      tok->destination = tokens[cur + 1];
      cur += 2;
      continue;
    }

    // --- Bridge top-level keywords ---
    if (kw == "member" && cur + 1 < tokens.size()) {
      if (!tok->bridge)
        tok->bridge.emplace();
      tok->bridge->members.push_back(tokens[cur + 1]);
      cur += 2;
      continue;
    }
    if (kw == "stp" && cur + 1 < tokens.size()) {
      if (!tok->bridge)
        tok->bridge.emplace();
      const std::string &val = tokens[cur + 1];
      tok->bridge->stp =
          (val == "on" || val == "yes" || val == "true" || val == "enable");
      cur += 2;
      continue;
    }
    if (kw == "priority" && cur + 1 < tokens.size()) {
      if (!tok->bridge)
        tok->bridge.emplace();
      tok->bridge->priority = std::stoi(tokens[cur + 1]);
      cur += 2;
      continue;
    }

    // --- Top-level VLAN keywords (outside vlan sub-block) ---
    if (kw == "vid" && cur + 1 < tokens.size()) {
      if (!tok->vlan) {
        tok->vlan.emplace();
        tok->vlan->name = tok->name();
      }
      tok->vlan->id = static_cast<uint16_t>(std::stoi(tokens[cur + 1]));
      cur += 2;
      continue;
    }
    if (kw == "parent" && cur + 1 < tokens.size()) {
      // Dual-use: populate VLAN parent if vlan context exists, else WLAN parent
      if (tok->vlan) {
        tok->vlan->parent = tokens[cur + 1];
      } else {
        if (!tok->wlan)
          tok->wlan.emplace(InterfaceConfig{});
        tok->wlan->parent = tokens[cur + 1];
      }
      cur += 2;
      continue;
    }

    // --- Top-level protocol keyword (for lagg outside sub-block) ---
    if (kw == "protocol" && cur + 1 < tokens.size()) {
      const std::string &proto = tokens[cur + 1];
      if (!tok->lagg)
        tok->lagg.emplace();
      if (proto == "lacp")
        tok->lagg->protocol = LaggProtocol::LACP;
      else if (proto == "failover")
        tok->lagg->protocol = LaggProtocol::FAILOVER;
      else if (proto == "loadbalance")
        tok->lagg->protocol = LaggProtocol::LOADBALANCE;
      else if (proto == "roundrobin")
        tok->lagg->protocol = LaggProtocol::ROUNDROBIN;
      else if (proto == "broadcast")
        tok->lagg->protocol = LaggProtocol::BROADCAST;
      else if (proto == "none")
        tok->lagg->protocol = LaggProtocol::NONE;
      cur += 2;
      continue;
    }

    // --- VXLAN keywords ---
    if (kw == "vni" && cur + 1 < tokens.size()) {
      if (!tok->vxlan)
        tok->vxlan.emplace(InterfaceConfig{});
      tok->vxlan->vni =
          static_cast<uint32_t>(std::stoul(tokens[cur + 1]));
      cur += 2;
      continue;
    }
    if (kw == "local" && cur + 1 < tokens.size()) {
      if (!tok->vxlan)
        tok->vxlan.emplace(InterfaceConfig{});
      tok->vxlan->localAddr = tokens[cur + 1];
      cur += 2;
      continue;
    }
    if (kw == "remote" && cur + 1 < tokens.size()) {
      if (!tok->vxlan)
        tok->vxlan.emplace(InterfaceConfig{});
      tok->vxlan->remoteAddr = tokens[cur + 1];
      cur += 2;
      continue;
    }
    if (kw == "port" && cur + 1 < tokens.size()) {
      if (!tok->vxlan)
        tok->vxlan.emplace(InterfaceConfig{});
      tok->vxlan->localPort =
          static_cast<uint16_t>(std::stoi(tokens[cur + 1]));
      cur += 2;
      continue;
    }

    // --- WLAN keywords ---
    if (kw == "ssid" && cur + 1 < tokens.size()) {
      if (!tok->wlan)
        tok->wlan.emplace(InterfaceConfig{});
      tok->wlan->ssid = tokens[cur + 1];
      cur += 2;
      continue;
    }
    if (kw == "channel" && cur + 1 < tokens.size()) {
      if (!tok->wlan)
        tok->wlan.emplace(InterfaceConfig{});
      tok->wlan->channel = std::stoi(tokens[cur + 1]);
      cur += 2;
      continue;
    }
    if (kw == "authmode" && cur + 1 < tokens.size()) {
      if (!tok->wlan)
        tok->wlan.emplace(InterfaceConfig{});
      const std::string &mode = tokens[cur + 1];
      if (mode == "open")
        tok->wlan->authmode = WlanAuthMode::OPEN;
      else if (mode == "shared")
        tok->wlan->authmode = WlanAuthMode::SHARED;
      else if (mode == "wpa" || mode == "wpa2")
        tok->wlan->authmode = WlanAuthMode::WPA;
      cur += 2;
      continue;
    }

    // --- GRE / CARP key keyword ---
    if (kw == "key" && cur + 1 < tokens.size()) {
      // Numeric → GRE key; non-numeric → CARP passphrase
      try {
        uint32_t k = static_cast<uint32_t>(std::stoul(tokens[cur + 1]));
        if (!tok->gre)
          tok->gre.emplace(InterfaceConfig{});
        tok->gre->greKey = k;
      } catch (...) {
        if (!tok->carp)
          tok->carp.emplace(InterfaceConfig{});
        tok->carp->key = tokens[cur + 1];
      }
      cur += 2;
      continue;
    }

    // --- CARP keywords ---
    if (kw == "vhid" && cur + 1 < tokens.size()) {
      if (!tok->carp)
        tok->carp.emplace(InterfaceConfig{});
      tok->carp->vhid = std::stoi(tokens[cur + 1]);
      cur += 2;
      continue;
    }
    if (kw == "advskew" && cur + 1 < tokens.size()) {
      if (!tok->carp)
        tok->carp.emplace(InterfaceConfig{});
      tok->carp->advskew = std::stoi(tokens[cur + 1]);
      cur += 2;
      continue;
    }
    if (kw == "advbase" && cur + 1 < tokens.size()) {
      if (!tok->carp)
        tok->carp.emplace(InterfaceConfig{});
      tok->carp->advbase = std::stoi(tokens[cur + 1]);
      cur += 2;
      continue;
    }

    // Unknown keyword; stop parsing
    break;
  }
}

std::shared_ptr<InterfaceToken>
InterfaceToken::parseFromTokens(const std::vector<std::string> &tokens,
                                size_t start, size_t &next) {
  next = start + 1; // by default consume the 'interfaces' token
  if (start + 1 < tokens.size()) {
    // There is at least one token after 'interfaces'
    const std::string &a = tokens[start + 1];
    const std::string b =
        (start + 2 < tokens.size()) ? tokens[start + 2] : std::string();

    // support `interfaces group <group>`
    if (a == "group") {
      if (!b.empty()) {
        std::string grp = b;
        size_t nnext = start + 3;
        auto tok = std::make_shared<InterfaceToken>(InterfaceType::Unknown,
                                                    std::string());
        tok->group = grp;
        next = nnext;
        return tok;
      }
    }

    // support `interfaces name <name>`
    if (a == "name") {
      if (!b.empty()) {
        std::string name = b;
        auto tok =
            std::make_shared<InterfaceToken>(InterfaceType::Unknown, name);
        size_t cur = start + 3;
        parseKeywords(tok, tokens, cur);
        next = cur;
        return tok;
      }
    }

    InterfaceType itype = InterfaceType::Unknown;
    std::string name;

    if (a == "type") {
      // form: interfaces type <type> [name]
      if (start + 2 >= tokens.size()) {
        // user typed 'type' but no value yet
        auto tok = std::make_shared<InterfaceToken>(InterfaceType::Unknown,
                                                    std::string());
        tok->expect_type_value_ = true;
        next = start + 2;
        return tok;
      }

      itype = interfaceTypeFromString(b);

      if (itype != InterfaceType::Unknown) {
        // optional name follows; allow `name <name>` form as well
        if (start + 3 < tokens.size()) {
          if (tokens[start + 3] == "name" && start + 4 < tokens.size())
            name = tokens[start + 4];
          else
            name = tokens[start + 3];
        }
        size_t cur =
            start + (name.empty() ? 3 : (tokens[start + 3] == "name" ? 5 : 4));

        auto tok = std::make_shared<InterfaceToken>(itype, name);
        parseKeywords(tok, tokens, cur);
        next = cur;
        return tok;
      }
    } else {
      // original form: interfaces <type> <name>
      itype = interfaceTypeFromString(a);
      if (itype != InterfaceType::Unknown && !b.empty()) {
        auto tok = std::make_shared<InterfaceToken>(itype, b);
        size_t cur = start + 3;
        parseKeywords(tok, tokens, cur);
        next = cur;
        return tok;
      }
    }
  }
  // bare interfaces
  return std::make_shared<InterfaceToken>(InterfaceType::Unknown,
                                          std::string());
}
