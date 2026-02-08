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

InterfaceToken::InterfaceToken(InterfaceType t, std::string name)
    : type_(t), name_(std::move(name)) {}

std::string InterfaceToken::toString() const {
  std::string result = "interface";

  // Always use "name <name>" format
  result += " name " + name_;

  // Add type keyword for interface types that need explicit creation
  if (type_ == InterfaceType::VLAN) {
    result += " type vlan";
  } else if (type_ == InterfaceType::Lagg) {
    result += " type lagg";
  } else if (type_ == InterfaceType::Bridge) {
    result += " type bridge";
  } else if (type_ == InterfaceType::Tunnel || type_ == InterfaceType::Gif) {
    result += " type tunnel";
  } else if (type_ == InterfaceType::Virtual) {
    result += " type epair";
  } else if (type_ == InterfaceType::Loopback) {
    result += " type loopback";
  }

  if (vrf) {
    result += " vrf " + std::to_string(*vrf);
  }
  if (address) {
    if (address_family) {
      result += (*address_family == AF_INET) ? " inet" : " inet6";
    }
    result += " address " + *address;
  }
  if (mtu) {
    result += " mtu " + std::to_string(*mtu);
  }
  if (vlan) {
    result += " vid " + std::to_string(vlan->id);
    if (vlan->parent) {
      result += " parent " + *vlan->parent;
    }
  }
  if (lagg) {
    for (const auto &member : lagg->members) {
      result += " member " + member;
    }
  }
  if (bridge) {
    if (bridge->stp) {
      result += " stp";
    }
    for (const auto &member : bridge->members) {
      result += " member " + member;
    }
  }
  if (tunnel) {
    if (tunnel->source) {
      result += " source " + tunnel->source->toString();
    }
    if (tunnel->destination) {
      result += " destination " + tunnel->destination->toString();
    }
  }
  if (tunnel_vrf && *tunnel_vrf != 0) {
    result += " tunnel-vrf " + std::to_string(*tunnel_vrf);
  }
  if (status) {
    result += *status ? " status up" : " status down";
  }

  if (next_) {
    result += " " + next_->toString();
  }
  return result;
}

std::vector<std::string> InterfaceToken::autoComplete(std::string_view) const {
  return {}; // suggestions could query ConfigurationManager; keep empty for
             // now
}

std::unique_ptr<Token> InterfaceToken::clone() const {
  return std::make_unique<InterfaceToken>(*this);
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
  if (start + 2 < tokens.size()) {
    // Support forms:
    //  - interfaces name <name>
    //  - interfaces <type> <name>
    //  - interfaces type <type> [name]
    const std::string &a = tokens[start + 1];
    const std::string &b = tokens[start + 2];

    // support `interfaces group <group>`
    if (a == "group") {
      std::string grp = b;
      size_t nnext = start + 3;
      auto tok = std::make_shared<InterfaceToken>(InterfaceType::Unknown,
                                                  std::string());
      tok->group = grp;
      next = nnext;
      return tok;
    }

    // support `interfaces name <name>`
    if (a == "name") {
      std::string name = b;
      auto tok = std::make_shared<InterfaceToken>(InterfaceType::Unknown, name);
      size_t cur = start + 3;
      parseKeywords(tok, tokens, cur);
      next = cur;
      return tok;
    }

    InterfaceType itype = InterfaceType::Unknown;
    std::string name;

    if (a == "type") {
      // form: interfaces type <type> [name]
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
      if (itype != InterfaceType::Unknown) {
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
