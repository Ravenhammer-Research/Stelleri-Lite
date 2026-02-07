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

#include "GenerateConfig.hpp"
#include "InterfaceToken.hpp"
#include "ProtocolsToken.hpp"
#include "RouteToken.hpp"
#include "SetToken.hpp"
#include "StaticToken.hpp"
#include "SystemConfigurationManager.hpp"
#include "VRFToken.hpp"
#include <iostream>
#include <set>

namespace netcli {

  static void generateVRFs(SystemConfigurationManager &mgr) {
    // Get the current net.fibs value from sysctl
    int fibs = mgr.GetFibs();
    if (fibs > 1) {
      std::cout << "set vrf fibs " << fibs << "\n";
    }
  }

  static void generateLoopbacks(SystemConfigurationManager &mgr,
                                std::set<std::string> &processedInterfaces) {
    auto interfaces = mgr.GetInterfaces();

    // Create loopback interfaces with first address
    for (const auto &ifc : interfaces) {
      if (processedInterfaces.count(ifc.name))
        continue;

      if (ifc.type != InterfaceType::Loopback)
        continue;

      auto setTok = std::make_shared<SetToken>();
      auto ifTok = std::make_shared<InterfaceToken>(ifc.type, ifc.name);

      if (ifc.address) {
        ifTok->address = ifc.address->toString();
        ifTok->address_family =
            (ifc.address->family() == AddressFamily::IPv4) ? AF_INET : AF_INET6;
      }
      if (ifc.vrf && ifc.vrf->table != 0)
        ifTok->vrf = ifc.vrf->table;
      if (ifc.mtu)
        ifTok->mtu = ifc.mtu;
      if (ifc.flags)
        ifTok->status = (*ifc.flags & 0x1) != 0; // IFF_UP = 0x1

      setTok->setNext(ifTok);
      std::cout << setTok->toString() << "\n";
      processedInterfaces.insert(ifc.name);

      // Output alias addresses as separate commands
      for (const auto &alias : ifc.aliases) {
        auto aliasTok = std::make_shared<SetToken>();
        auto aliasIfTok = std::make_shared<InterfaceToken>(ifc.type, ifc.name);
        aliasIfTok->address = alias->toString();
        aliasIfTok->address_family =
            (alias->family() == AddressFamily::IPv4) ? AF_INET : AF_INET6;

        aliasTok->setNext(aliasIfTok);
        std::cout << aliasTok->toString() << "\n";
      }
    }
  }

  static void
  generateBasicInterfaces(SystemConfigurationManager &mgr,
                          std::set<std::string> &processedInterfaces) {
    auto interfaces = mgr.GetInterfaces();

    // Create interfaces (only Ethernet - others have specialized handlers)
    for (const auto &ifc : interfaces) {
      if (processedInterfaces.count(ifc.name))
        continue;

      // Skip interface types that have specialized generation functions
      if (ifc.type != InterfaceType::Ethernet)
        continue;

      auto setTok = std::make_shared<SetToken>();
      auto ifTok = std::make_shared<InterfaceToken>(ifc.type, ifc.name);

      if (ifc.mtu)
        ifTok->mtu = ifc.mtu;
      if (ifc.vrf && ifc.vrf->table != 0)
        ifTok->vrf = ifc.vrf->table;
      if (ifc.flags)
        ifTok->status = (*ifc.flags & 0x1) != 0; // IFF_UP = 0x1

      setTok->setNext(ifTok);
      std::cout << setTok->toString() << "\n";
      processedInterfaces.insert(ifc.name);
    }

    // Assign addresses (only Ethernet)
    for (const auto &ifc : interfaces) {
      // Skip interface types that have specialized generation functions
      if (ifc.type != InterfaceType::Ethernet)
        continue;

      if (ifc.address) {
        auto setTok = std::make_shared<SetToken>();
        auto ifTok = std::make_shared<InterfaceToken>(ifc.type, ifc.name);
        ifTok->address = ifc.address->toString();
        ifTok->address_family =
            (ifc.address->family() == AddressFamily::IPv4) ? AF_INET : AF_INET6;

        setTok->setNext(ifTok);
        std::cout << setTok->toString() << "\n";
      }

      for (const auto &alias : ifc.aliases) {
        auto setTok = std::make_shared<SetToken>();
        auto ifTok = std::make_shared<InterfaceToken>(ifc.type, ifc.name);
        ifTok->address = alias->toString();
        ifTok->address_family =
            (alias->family() == AddressFamily::IPv4) ? AF_INET : AF_INET6;

        setTok->setNext(ifTok);
        std::cout << setTok->toString() << "\n";
      }
    }
  }

  static void generateBridges(SystemConfigurationManager &mgr,
                              std::set<std::string> &processedInterfaces) {
    auto bridges = mgr.GetBridgeInterfaces();

    for (const auto &ifc : bridges) {
      if (processedInterfaces.count(ifc.name))
        continue;

      auto setTok = std::make_shared<SetToken>();
      auto ifTok = std::make_shared<InterfaceToken>(ifc.type, ifc.name);
      ifTok->bridge.emplace(ifc);
      if (ifc.address) {
        ifTok->address = ifc.address->toString();
        ifTok->address_family =
            (ifc.address->family() == AddressFamily::IPv4) ? AF_INET : AF_INET6;
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
        auto aliasIfTok = std::make_shared<InterfaceToken>(ifc.type, ifc.name);
        aliasIfTok->bridge.emplace(ifc);
        aliasIfTok->address = alias->toString();
        aliasIfTok->address_family =
            (alias->family() == AddressFamily::IPv4) ? AF_INET : AF_INET6;

        aliasTok->setNext(aliasIfTok);
        std::cout << aliasTok->toString() << "\n";
      }
    }
  }

  static void generateLaggs(SystemConfigurationManager &mgr,
                            std::set<std::string> &processedInterfaces) {
    auto laggs = mgr.GetLaggInterfaces();

    for (const auto &ifc : laggs) {
      if (processedInterfaces.count(ifc.name))
        continue;

      auto setTok = std::make_shared<SetToken>();
      auto ifTok = std::make_shared<InterfaceToken>(ifc.type, ifc.name);
      ifTok->lagg.emplace(ifc);
      if (ifc.address) {
        ifTok->address = ifc.address->toString();
        ifTok->address_family =
            (ifc.address->family() == AddressFamily::IPv4) ? AF_INET : AF_INET6;
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
        auto aliasIfTok = std::make_shared<InterfaceToken>(ifc.type, ifc.name);
        aliasIfTok->lagg.emplace(ifc);
        aliasIfTok->address = alias->toString();
        aliasIfTok->address_family =
            (alias->family() == AddressFamily::IPv4) ? AF_INET : AF_INET6;

        aliasTok->setNext(aliasIfTok);
        std::cout << aliasTok->toString() << "\n";
      }
    }
  }

  static void generateVLANs(SystemConfigurationManager &mgr,
                            std::set<std::string> &processedInterfaces) {
    auto vlans = mgr.GetVLANInterfaces();

    for (const auto &ifc : vlans) {
      const std::string &ifname = ifc.name;
      const InterfaceType iftype =
          static_cast<const InterfaceConfig &>(ifc).type;
      if (processedInterfaces.count(ifname))
        continue;

      auto setTok = std::make_shared<SetToken>();
      auto ifTok = std::make_shared<InterfaceToken>(iftype, ifname);
      ifTok->vlan.emplace(ifc);
      if (ifc.address) {
        ifTok->address = ifc.address->toString();
        ifTok->address_family =
            (ifc.address->family() == AddressFamily::IPv4) ? AF_INET : AF_INET6;
      }
      if (ifc.vrf && ifc.vrf->table != 0)
        ifTok->vrf = ifc.vrf->table;
      if (ifc.flags)
        ifTok->status = (*ifc.flags & 0x1) != 0;

      setTok->setNext(ifTok);
      std::cout << setTok->toString() << "\n";
      processedInterfaces.insert(ifname);

      // Output aliases as separate commands
      for (const auto &alias : ifc.aliases) {
        auto aliasTok = std::make_shared<SetToken>();
        auto aliasIfTok = std::make_shared<InterfaceToken>(iftype, ifname);
        aliasIfTok->vlan.emplace(ifc);
        aliasIfTok->address = alias->toString();
        aliasIfTok->address_family =
            (alias->family() == AddressFamily::IPv4) ? AF_INET : AF_INET6;

        aliasTok->setNext(aliasIfTok);
        std::cout << aliasTok->toString() << "\n";
      }
    }
  }

  static void generateTunnels(SystemConfigurationManager &mgr,
                              std::set<std::string> &processedInterfaces) {
    auto tunnels = mgr.GetTunnelInterfaces();

    for (const auto &ifc : tunnels) {
      if (processedInterfaces.count(ifc.name))
        continue;

      const InterfaceType iftype =
          static_cast<const InterfaceConfig &>(ifc).type;
      auto setTok = std::make_shared<SetToken>();
      auto ifTok = std::make_shared<InterfaceToken>(iftype, ifc.name);
      ifTok->tunnel.emplace(ifc);
      if (ifc.address) {
        ifTok->address = ifc.address->toString();
        ifTok->address_family =
            (ifc.address->family() == AddressFamily::IPv4) ? AF_INET : AF_INET6;
      }
      if (ifc.vrf && ifc.vrf->table != 0)
        ifTok->vrf = ifc.vrf->table;
      if (ifc.tunnel_vrf && *ifc.tunnel_vrf != 0)
        ifTok->tunnel_vrf = ifc.tunnel_vrf;
      if (ifc.flags)
        ifTok->status = (*ifc.flags & 0x1) != 0;

      setTok->setNext(ifTok);
      std::cout << setTok->toString() << "\n";
      processedInterfaces.insert(ifc.name);

      // Output aliases as separate commands
      for (const auto &alias : ifc.aliases) {
        auto aliasTok = std::make_shared<SetToken>();
        auto aliasIfTok = std::make_shared<InterfaceToken>(iftype, ifc.name);
        aliasIfTok->tunnel.emplace(ifc);
        aliasIfTok->address = alias->toString();
        aliasIfTok->address_family =
            (alias->family() == AddressFamily::IPv4) ? AF_INET : AF_INET6;

        aliasTok->setNext(aliasIfTok);
        std::cout << aliasTok->toString() << "\n";
      }
    }
  }

  static void generateVirtuals(SystemConfigurationManager &mgr,
                               std::set<std::string> &processedInterfaces) {
    auto virtuals = mgr.GetVirtualInterfaces();

    for (const auto &ifc : virtuals) {
      processedInterfaces.insert(ifc.name);

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

  static void generateRoutes(SystemConfigurationManager &mgr) {
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

  void generateConfiguration() {
    SystemConfigurationManager mgr;
    std::set<std::string> processedInterfaces;

    // Generate VRFs
    generateVRFs(mgr);

    // Generate interfaces with addresses
    generateLoopbacks(mgr, processedInterfaces);
    generateBasicInterfaces(mgr, processedInterfaces);
    generateBridges(mgr, processedInterfaces);
    generateLaggs(mgr, processedInterfaces);
    generateVLANs(mgr, processedInterfaces);
    generateTunnels(mgr, processedInterfaces);
    generateVirtuals(mgr, processedInterfaces);

    // Generate routes
    generateRoutes(mgr);
  }

} // namespace netcli
