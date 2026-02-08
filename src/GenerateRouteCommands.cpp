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

#include "CommandGenerator.hpp"
#include "InterfaceToken.hpp"
#include "RouteToken.hpp"
#include "SetToken.hpp"
#include "VRFToken.hpp"
#include <iostream>

namespace netcli {

  void CommandGenerator::generateRoutes(ConfigurationManager &mgr) {
    auto routes = mgr.GetRoutes();

    for (const auto &route : routes) {
      auto setTok = std::make_shared<SetToken>();
      auto routeTok = std::make_shared<RouteToken>(route.prefix);

      if (route.nexthop) {
        routeTok->nexthop = IPAddress::fromString(*route.nexthop);
      }
      if (route.iface) {
        routeTok->interface = std::make_unique<InterfaceToken>(
            InterfaceType::Unknown, *route.iface);
      }
      if (route.vrf && *route.vrf != 0) {
        routeTok->vrf = std::make_unique<VRFToken>(*route.vrf);
      }
      routeTok->blackhole = route.blackhole;
      routeTok->reject = route.reject;

      setTok->setNext(routeTok);
      std::cout << setTok->toString() << "\n";
    }
  }

  void CommandGenerator::generateVirtuals(
      ConfigurationManager &mgr, std::set<std::string> &processedInterfaces) {
    auto virtuals = mgr.GetVirtualInterfaces();

    for (const auto &ifc : virtuals) {
      if (processedInterfaces.count(ifc.name))
        continue;

      // Skip epair interfaces (handled separately)
      bool is_epair = false;
      for (const auto &g : ifc.groups) {
        if (g == "epair") {
          is_epair = true;
          break;
        }
      }
      if (is_epair)
        continue;

      // Only output if there's an address to assign
      if (ifc.address || !ifc.aliases.empty()) {
        const InterfaceType iftype =
            static_cast<const InterfaceConfig &>(ifc).type;
        auto setTok = std::make_shared<SetToken>();
        auto ifTok = std::make_shared<InterfaceToken>(iftype, ifc.name);
        if (ifc.address) {
          ifTok->address = ifc.address->toString();
          ifTok->address_family = (ifc.address->family() == AddressFamily::IPv4)
                                      ? AF_INET
                                      : AF_INET6;
        }
        if (ifc.vrf && ifc.vrf->table != 0)
          ifTok->vrf = ifc.vrf->table;
        if (ifc.flags)
          ifTok->status = (*ifc.flags & 0x1) != 0;

        setTok->setNext(ifTok);
        std::cout << setTok->toString() << "\n";
        processedInterfaces.insert(ifc.name);

        // Output aliases as separate commands
        for (const auto &alias : ifc.aliases) {
          auto aliasTok = std::make_shared<SetToken>();
          auto aliasIfTok = std::make_shared<InterfaceToken>(iftype, ifc.name);
          aliasIfTok->address = alias->toString();
          aliasIfTok->address_family =
              (alias->family() == AddressFamily::IPv4) ? AF_INET : AF_INET6;

          aliasTok->setNext(aliasIfTok);
          std::cout << aliasTok->toString() << "\n";
        }
      }
    }
  }

} // namespace netcli
