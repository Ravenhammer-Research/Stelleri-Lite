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
#include "InterfaceFlags.hpp"
#include "InterfaceTableFormatter.hpp"
#include "InterfaceType.hpp"
#include "LaggTableFormatter.hpp"
#include "SingleInterfaceSummaryFormatter.hpp"
#include "TunnelTableFormatter.hpp"
#include "VLANTableFormatter.hpp"
#include "VirtualTableFormatter.hpp"
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <unordered_set>
#include "InterfaceConfig.hpp"
#include "BridgeConfig.hpp"
#include "CarpConfig.hpp"
#include "GREConfig.hpp"
#include "LaggConfig.hpp"
#include "SixToFourConfig.hpp"
#include "TapConfig.hpp"
#include "TunnelConfig.hpp"
#include "VLANConfig.hpp"
#include "VXLANConfig.hpp"
#include "WlanConfig.hpp"

InterfaceToken::InterfaceToken(InterfaceType t, std::string name)
    : type_(t), name_(std::move(name)) {}

// toString(ConfigData*) removed — implementation deleted per request

std::vector<std::string> InterfaceToken::autoComplete(std::string_view partial) const {
  // If user typed `type` and is now completing the type value, suggest types
  if (expect_type_value_) {
    std::vector<std::string> types = {"ethernet", "loopback", "bridge", "lagg", "vlan", "tunnel", "epair", "virtual", "wireless", "gre", "gif", "vxlan"};
    std::vector<std::string> matches;
    for (const auto &t : types) {
      if (t.rfind(partial, 0) == 0)
        matches.push_back(t);
    }
    return matches;
  }

  // If we are at the top-level after `show interface`, only suggest these
  // primary keywords.
  if (name_.empty() && type_ == InterfaceType::Unknown && !vrf && !mtu && !status && !vlan && !lagg && !bridge && !tunnel) {
    std::vector<std::string> top = {"name", "group", "type"};
    std::vector<std::string> matches;
    for (const auto &opt : top) {
      if (opt.rfind(partial, 0) == 0)
        matches.push_back(opt);
    }
    return matches;
  }

  // If a name is already set but no other attributes, only suggest
  // type and group as the next keywords.
  if (!name_.empty() && type_ == InterfaceType::Unknown && !vrf && !mtu &&
      !status && !vlan && !lagg && !bridge && !tunnel) {
    std::vector<std::string> next_opts = {"type", "group"};
    std::vector<std::string> matches;
    for (const auto &opt : next_opts) {
      if (opt.rfind(partial, 0) == 0)
        matches.push_back(opt);
    }
    return matches;
  }

  // Fallback: suggest the usual wide set
  std::vector<std::string> options = {
      "name", "type", "inet", "inet6", "address", "mtu", "vrf", "status",
      "up", "down", "group", "vid", "parent", "member", "source",
      "destination", "tunnel-vrf", "protocol", "stp",
      // Type values
      "ethernet", "loopback", "bridge", "lagg", "vlan", "tunnel",
      "epair", "virtual", "wireless", "gre", "gif", "vxlan",
      // Lagg protocols
      "lacp", "failover", "loadbalance", "roundrobin", "broadcast"
  };
  std::vector<std::string> matches;
  for (const auto &opt : options) {
    if (opt.rfind(partial, 0) == 0)
      matches.push_back(opt);
  }
  return matches;
}

std::unique_ptr<Token> InterfaceToken::clone() const {
  return std::make_unique<InterfaceToken>(*this);
}

std::vector<std::string> InterfaceToken::autoComplete(
    const std::vector<std::string> &tokens, std::string_view partial,
    ConfigurationManager *mgr) const {
  std::vector<std::string> matches;
  if (!mgr)
    return matches;

  // If completing a group value (tokens end with 'group'), list all groups
  if (!tokens.empty() && tokens.back() == "group") {
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

  // If completing an interface name (after the 'name' keyword), list names
  if (!tokens.empty() && tokens.back() == "name") {
    auto ifs = mgr->GetInterfaces();
    for (const auto &i : ifs) {
      const std::string &iname = i.name;
      if (iname.rfind(partial, 0) == 0)
        matches.push_back(iname);
    }
    return matches;
  }

  // If the previous token is a known interface name (i.e. we are past
  // "interface name <ifname>"), only suggest type and group.
  if (tokens.size() >= 2 && tokens[tokens.size() - 2] == "name") {
    std::vector<std::string> next_opts = {"type", "group"};
    for (const auto &opt : next_opts) {
      if (opt.rfind(partial, 0) == 0)
        matches.push_back(opt);
    }
    return matches;
  }

  // Fallback to default behavior
  return autoComplete(partial);
}

// Static renderers for interface configs
std::string InterfaceToken::toString(InterfaceConfig *cfg) {
  if (!cfg) return std::string();
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
  if (cfg->vrf) result += " vrf " + std::to_string(cfg->vrf->table);
  if (cfg->mtu) result += " mtu " + std::to_string(*cfg->mtu);
  if (!cfg->groups.empty()) {
    for (const auto &g : cfg->groups) result += " group " + g;
  }
  return result;
}

std::string InterfaceToken::toString(BridgeConfig *cfg) {
  if (!cfg) return std::string();
  return std::string("interface type bridge");
}

std::string InterfaceToken::toString(CarpConfig *cfg) {
  if (!cfg) return std::string();
  std::string s = InterfaceToken::toString(static_cast<InterfaceConfig *>(cfg));
  if (cfg->vhid) s += " vhid " + std::to_string(*cfg->vhid);
  if (cfg->advskew) s += " advskew " + std::to_string(*cfg->advskew);
  if (cfg->advbase) s += " advbase " + std::to_string(*cfg->advbase);
  if (cfg->state) s += " state " + *cfg->state;
  return s;
}

std::string InterfaceToken::toString(GREConfig *cfg) {
  if (!cfg) return std::string();
  std::string s = InterfaceToken::toString(static_cast<InterfaceConfig *>(cfg));
  if (cfg->greSource) s += " source " + *cfg->greSource;
  if (cfg->greDestination) s += " destination " + *cfg->greDestination;
  return s;
}

std::string InterfaceToken::toString(LaggConfig *cfg) {
  if (!cfg) return std::string();
  std::string s = InterfaceToken::toString(static_cast<InterfaceConfig *>(cfg));
  for (const auto &m : cfg->members) s += " member " + m;
  return s;
}

std::string InterfaceToken::toString(SixToFourConfig *cfg) {
  if (!cfg) return std::string();
  return InterfaceToken::toString(static_cast<InterfaceConfig *>(cfg));
}

std::string InterfaceToken::toString(TapConfig *cfg) {
  if (!cfg) return std::string();
  return InterfaceToken::toString(static_cast<InterfaceConfig *>(cfg));
}

std::string InterfaceToken::toString(TunnelConfig *cfg) {
  if (!cfg) return std::string();
  std::string s = InterfaceToken::toString(static_cast<InterfaceConfig *>(cfg));
  if (cfg->source) s += " source " + cfg->source->toString();
  if (cfg->destination) s += " destination " + cfg->destination->toString();
  return s;
}

std::string InterfaceToken::toString(VLANConfig *cfg) {
  if (!cfg) return std::string();
  std::string s = InterfaceToken::toString(static_cast<InterfaceConfig *>(cfg));
  s += " vid " + std::to_string(cfg->id);
  if (cfg->parent) s += " parent " + *cfg->parent;
  return s;
}

std::string InterfaceToken::toString(VXLANConfig *cfg) {
  if (!cfg) return std::string();
  std::string s = InterfaceToken::toString(static_cast<InterfaceConfig *>(cfg));
  if (cfg->vni) s += " vni " + std::to_string(*cfg->vni);
  if (cfg->localAddr) s += " local " + *cfg->localAddr;
  if (cfg->remoteAddr) s += " remote " + *cfg->remoteAddr;
  return s;
}

std::string InterfaceToken::toString(WlanConfig *cfg) {
  if (!cfg) return std::string();
  std::string s = InterfaceToken::toString(static_cast<InterfaceConfig *>(cfg));
  if (cfg->ssid) s += " ssid " + *cfg->ssid;
  if (cfg->channel) s += " channel " + std::to_string(*cfg->channel);
  if (cfg->parent) s += " parent " + *cfg->parent;
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
      LaggConfig lc;
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
    const std::string b = (start + 2 < tokens.size()) ? tokens[start + 2] : std::string();

    // support `interfaces group <group>`
    if (a == "group") {
      if (!b.empty()) {
        std::string grp = b;
        size_t nnext = start + 3;
        auto tok = std::make_shared<InterfaceToken>(InterfaceType::Unknown, std::string());
        tok->group = grp;
        next = nnext;
        return tok;
      }
    }

    // support `interfaces name <name>`
    if (a == "name") {
      if (!b.empty()) {
        std::string name = b;
        auto tok = std::make_shared<InterfaceToken>(InterfaceType::Unknown, name);
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
        auto tok = std::make_shared<InterfaceToken>(InterfaceType::Unknown, std::string());
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
        size_t cur = start + (name.empty() ? 3 : (tokens[start + 3] == "name" ? 5 : 4));

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
