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

#include "BridgeTableFormatter.hpp"
#include "CarpTableFormatter.hpp"
#include "ConfigurationManager.hpp"
#include "InterfaceTableFormatter.hpp"
#include "InterfaceToken.hpp"
#include "LaggTableFormatter.hpp"
#include "Parser.hpp"
#include "SingleInterfaceSummaryFormatter.hpp"
#include "SixToFourTableFormatter.hpp"
#include "TapTableFormatter.hpp"
#include "TunnelTableFormatter.hpp"
#include "VLANTableFormatter.hpp"
#include "VirtualTableFormatter.hpp"
#include "WlanTableFormatter.hpp"
#include <iomanip>
#include <iostream>
#include <sstream>

void netcli::Parser::executeShowInterface(const InterfaceToken &tok,
                                          ConfigurationManager *mgr) const {
  if (!mgr) {
    std::cout << "No ConfigurationManager provided\n";
    return;
  }
  std::vector<ConfigData> interfaces;
  if (!tok.name().empty()) {
    // Prefer to query the ConfigurationManager for the named interface
    auto cdopt = mgr->getInterface(tok.name());
    if (cdopt) {
      if (cdopt->iface && cdopt->iface->type == InterfaceType::Bridge) {
        // Retrieve bridge-specific representation (members, etc.)
        auto brs = mgr->GetBridgeInterfaces();
        for (auto &b : brs) {
          if (b.name == tok.name()) {
            ConfigData cd;
            cd.iface = std::make_shared<BridgeInterfaceConfig>(std::move(b));
            interfaces.push_back(std::move(cd));
            break;
          }
        }
      } else {
        interfaces.push_back(std::move(*cdopt));
      }
    }
  } else if (tok.type() != InterfaceType::Unknown) {
    if (tok.type() == InterfaceType::Bridge) {
      auto brs = mgr->GetBridgeInterfaces();
      for (auto &b : brs) {
        if (tok.group && std::find(b.groups.begin(), b.groups.end(),
                                   *tok.group) == b.groups.end())
          continue;
        ConfigData cd;
        cd.iface = std::make_shared<BridgeInterfaceConfig>(std::move(b));
        interfaces.push_back(std::move(cd));
      }
    } else if (tok.type() == InterfaceType::Lagg) {
      auto lgs = mgr->GetLaggInterfaces();
      for (auto &l : lgs) {
        if (tok.group && std::find(l.groups.begin(), l.groups.end(),
                                   *tok.group) == l.groups.end())
          continue;
        ConfigData cd;
        cd.iface = std::make_shared<LaggConfig>(std::move(l));
        interfaces.push_back(std::move(cd));
      }
    } else if (tok.type() == InterfaceType::VLAN) {
      auto vls = mgr->GetVLANInterfaces();
      for (auto &v : vls) {
        if (tok.group && std::find(v.groups.begin(), v.groups.end(),
                                   *tok.group) == v.groups.end())
          continue;
        ConfigData cd;
        cd.iface = std::make_shared<VLANConfig>(std::move(v));
        interfaces.push_back(std::move(cd));
      }
    } else if (tok.type() == InterfaceType::Tunnel ||
               tok.type() == InterfaceType::Gif ||
               tok.type() == InterfaceType::Tun) {
      auto tfs = mgr->GetTunnelInterfaces();
      for (auto &t : tfs) {
        if (tok.group && std::find(t.groups.begin(), t.groups.end(),
                                   *tok.group) == t.groups.end())
          continue;
        ConfigData cd;
        cd.iface = std::make_shared<TunnelConfig>(std::move(t));
        interfaces.push_back(std::move(cd));
      }
    } else if (tok.type() == InterfaceType::Virtual) {
      auto vifs = mgr->GetVirtualInterfaces();
      for (auto &v : vifs) {
        if (tok.group && std::find(v.groups.begin(), v.groups.end(),
                                   *tok.group) == v.groups.end())
          continue;
        ConfigData cd;
        cd.iface = std::make_shared<VirtualInterfaceConfig>(std::move(v));
        interfaces.push_back(std::move(cd));
      }
    } else {
      // Fallback: inspect all interfaces and select those matching type and
      // group
      auto allIfaces = mgr->getInterfaces();
      for (auto &iface : allIfaces) {
        if (!iface.iface)
          continue;
        if (tok.group) {
          bool has = false;
          for (const auto &g : iface.iface->groups) {
            if (g == *tok.group) {
              has = true;
              break;
            }
          }
          if (!has)
            continue;
        }
        if (tok.type() == InterfaceType::Tunnel ||
            tok.type() == InterfaceType::Gif ||
            tok.type() == InterfaceType::Tun) {
          if (iface.iface->type == InterfaceType::Tunnel ||
              iface.iface->type == InterfaceType::Gif ||
              iface.iface->type == InterfaceType::Tun) {
            interfaces.push_back(std::move(iface));
          }
          continue;
        }

        if (iface.iface->type == tok.type())
          interfaces.push_back(std::move(iface));
      }
    }
  } else {
    if (tok.group) {
      // Filter the main interfaces table by group
      auto ifs = mgr->GetInterfacesByGroup(std::nullopt, *tok.group);
      for (auto &ic : ifs) {
        ConfigData cd;
        cd.iface = std::make_shared<InterfaceConfig>(std::move(ic));
        interfaces.push_back(std::move(cd));
      }
    } else {
      interfaces = mgr->getInterfaces();
    }
  }

  // If the request was 'show interface group <g>' with no explicit type,
  // prefer the generic interfaces table rather than type-specific tables
  // (e.g., avoid using VirtualTableFormatter for epair group queries).
  if (tok.group && tok.type() == InterfaceType::Unknown) {
    InterfaceTableFormatter formatter;
    std::cout << formatter.format(interfaces);
    return;
  }

  if (interfaces.size() == 1 && !tok.name().empty()) {
    SingleInterfaceSummaryFormatter formatter;
    std::cout << formatter.format(interfaces[0]);
    return;
  }

  bool allBridge = true;
  bool allLagg = true;
  bool allVlan = true;
  bool allTunnel = true;
  bool allVirtual = true;
  bool allWlan = true;
  bool allSixToFour = true;
  bool allTap = true;
  bool allCarp = true;

  for (const auto &cd : interfaces) {
    if (!cd.iface)
      continue;
    if (cd.iface->type != InterfaceType::Bridge)
      allBridge = false;
    if (cd.iface->type != InterfaceType::Lagg)
      allLagg = false;
    if (cd.iface->type != InterfaceType::VLAN)
      allVlan = false;
    // Treat explicit tunnel-ish interface types as tunnels for formatting
    if (!(cd.iface->type == InterfaceType::Tunnel ||
          cd.iface->type == InterfaceType::Gif ||
          cd.iface->type == InterfaceType::Tun))
      allTunnel = false;
    if (cd.iface->type != InterfaceType::Virtual)
      allVirtual = false;
    if (cd.iface->type != InterfaceType::Wireless)
      allWlan = false;
    // SixToFour heuristics: treat tunnel-like interfaces whose name begins with
    // gif/stf/sit as six-to-four style
    if (!(cd.iface->type == InterfaceType::Tunnel ||
          cd.iface->type == InterfaceType::Gif ||
          cd.iface->type == InterfaceType::Tun))
      allSixToFour = false;
    if (!(cd.iface->name.rfind("gif", 0) == 0 ||
          cd.iface->name.rfind("stf", 0) == 0 ||
          cd.iface->name.rfind("sit", 0) == 0))
      allSixToFour = false;
    // Tap heuristics: name starts with "tap" or treated as virtual
    if (cd.iface->type != InterfaceType::Virtual &&
        cd.iface->name.rfind("tap", 0) != 0)
      allTap = false;
    // CARP heuristics: name starts with carp/ vh or virtual
    if (cd.iface->name.rfind("carp", 0) != 0 &&
        cd.iface->name.rfind("vh", 0) != 0 &&
        cd.iface->type != InterfaceType::Virtual)
      allCarp = false;
  }

  if (allBridge && !interfaces.empty()) {
    BridgeTableFormatter formatter;
    std::cout << formatter.format(interfaces);
    return;
  } else if (allLagg && !interfaces.empty()) {
    LaggTableFormatter formatter;
    std::cout << formatter.format(interfaces);
    return;
  } else if (allVlan && !interfaces.empty()) {
    VLANTableFormatter formatter;
    std::cout << formatter.format(interfaces);
    return;
  } else if (allWlan && !interfaces.empty()) {
    WlanTableFormatter formatter;
    std::cout << formatter.format(interfaces);
    return;
  } else if (allTunnel && !interfaces.empty()) {
    TunnelTableFormatter formatter;
    std::cout << formatter.format(interfaces);
    return;
  } else if (allSixToFour && !interfaces.empty()) {
    SixToFourTableFormatter formatter;
    std::cout << formatter.format(interfaces);
    return;
  } else if (allVirtual && !interfaces.empty()) {
    VirtualTableFormatter formatter;
    std::cout << formatter.format(interfaces);
    return;
  } else if (allTap && !interfaces.empty()) {
    TapTableFormatter formatter;
    std::cout << formatter.format(interfaces);
    return;
  } else if (allCarp && !interfaces.empty()) {
    CarpTableFormatter formatter;
    std::cout << formatter.format(interfaces);
    return;
  }

  InterfaceTableFormatter formatter;
  std::cout << formatter.format(interfaces);
}
