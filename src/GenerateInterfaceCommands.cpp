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
#include "InterfaceFlags.hpp"
#include "InterfaceToken.hpp"
#include "SetToken.hpp"
#include <iostream>

namespace netcli {

  void CommandGenerator::generateBasicInterfaces(
      ConfigurationManager &mgr, std::set<std::string> &processedInterfaces) {
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
        ifTok->status = hasFlag(*ifc.flags, InterfaceFlag::UP);

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

} // namespace netcli
