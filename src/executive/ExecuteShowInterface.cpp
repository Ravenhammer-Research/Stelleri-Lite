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
#include "GreTableFormatter.hpp"
#include "InterfaceConfig.hpp"
#include "InterfaceTableFormatter.hpp"
#include "InterfaceToken.hpp"
#include "LaggTableFormatter.hpp"
#include "SingleInterfaceSummaryFormatter.hpp"
#include "SixToFourTableFormatter.hpp"
#include "TapTableFormatter.hpp"
#include "TunTableFormatter.hpp"
#include "GifTableFormatter.hpp"
#include "OvpnTableFormatter.hpp"
#include "IpsecTableFormatter.hpp"
#include "VlanTableFormatter.hpp"
#include "VxlanTableFormatter.hpp"
#include "EpairTableFormatter.hpp"
#include "WlanTableFormatter.hpp"
#include <iomanip>
#include <iostream>
#include <sstream>

namespace netcli {

  void executeShowInterface(const InterfaceToken &tok,
                            ConfigurationManager *mgr) {
    if (!mgr) {
      std::cout << "No ConfigurationManager provided\n";
      return;
    }
    std::vector<InterfaceConfig> interfaces;
    if (!tok.name().empty()) {
      // Prefer to query the ConfigurationManager for the named interface
      auto ifopt = mgr->GetInterface(tok.name());
      if (ifopt) {
        interfaces.push_back(std::move(*ifopt));
      }
    } else if (tok.type() != InterfaceType::Unknown) {
      // Get all interfaces and filter by type and group
      auto allIfaces = mgr->GetInterfaces();
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
        if (iface.matchesType(tok.type()))
          interfaces.push_back(std::move(iface));
      }
    } else {
      if (tok.group) {
        // Filter the main interfaces table by group
        interfaces = mgr->GetInterfacesByGroup(std::nullopt, *tok.group);
      } else {
        interfaces = mgr->GetInterfaces();
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
      SingleInterfaceSummaryFormatter formatter(mgr);
      std::cout << formatter.format(interfaces[0]);
      return;
    }

    std::cout << InterfaceConfig::formatInterfaces(interfaces, mgr);
  }
} // namespace netcli
