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

#include "BridgeInterfaceConfig.hpp"
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
  std::vector<InterfaceConfig> interfaces;
  if (!tok.name().empty()) {
    // Prefer to query the ConfigurationManager for the named interface
    auto ifopt = mgr->getInterface(tok.name());
    if (ifopt) {
      interfaces.push_back(std::move(*ifopt));
    }
  } else if (tok.type() != InterfaceType::Unknown) {
    // Get all interfaces and filter by type and group
    auto allIfaces = mgr->getInterfaces();
    for (auto &iface : allIfaces) {
      if (tok.group) {
        bool has = false;
        for (const auto &g : iface.groups) {
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
        if (iface.type == InterfaceType::Tunnel ||
            iface.type == InterfaceType::Gif ||
            iface.type == InterfaceType::Tun) {
          interfaces.push_back(std::move(iface));
        }
        continue;
      }

      if (iface.type == tok.type())
        interfaces.push_back(std::move(iface));
    }
  } else {
    if (tok.group) {
      // Filter the main interfaces table by group
      interfaces = mgr->GetInterfacesByGroup(std::nullopt, *tok.group);
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

  for (const auto &iface : interfaces) {
    if (iface.type != InterfaceType::Bridge)
      allBridge = false;
    if (iface.type != InterfaceType::Lagg)
      allLagg = false;
    if (iface.type != InterfaceType::VLAN)
      allVlan = false;
    // Treat explicit tunnel-ish interface types as tunnels for formatting
    if (!(iface.type == InterfaceType::Tunnel ||
          iface.type == InterfaceType::Gif || iface.type == InterfaceType::Tun))
      allTunnel = false;
    if (iface.type != InterfaceType::Virtual)
      allVirtual = false;
    if (iface.type != InterfaceType::Wireless)
      allWlan = false;
    // SixToFour heuristics: treat tunnel-like interfaces whose name begins with
    // gif/stf/sit as six-to-four style
    if (!(iface.type == InterfaceType::Tunnel ||
          iface.type == InterfaceType::Gif || iface.type == InterfaceType::Tun))
      allSixToFour = false;
    if (!(iface.name.rfind("gif", 0) == 0 || iface.name.rfind("stf", 0) == 0 ||
          iface.name.rfind("sit", 0) == 0))
      allSixToFour = false;
    // Tap heuristics: name starts with "tap" or treated as virtual
    if (iface.type != InterfaceType::Virtual && iface.name.rfind("tap", 0) != 0)
      allTap = false;
    // CARP heuristics: name starts with carp/ vh or virtual
    if (iface.name.rfind("carp", 0) != 0 && iface.name.rfind("vh", 0) != 0 &&
        iface.type != InterfaceType::Virtual)
      allCarp = false;
  }

  if (allBridge && !interfaces.empty()) {
    // Build vector of BridgeInterfaceConfig
    auto bridgeIfaces = mgr->GetBridgeInterfaces();
    std::vector<BridgeInterfaceConfig> bridgeVec;
    for (const auto &iface : interfaces) {
      for (auto &b : bridgeIfaces) {
        if (b.name == iface.name) {
          bridgeVec.push_back(std::move(b));
          break;
        }
      }
    }
    BridgeTableFormatter formatter;
    std::cout << formatter.format(bridgeVec);
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
