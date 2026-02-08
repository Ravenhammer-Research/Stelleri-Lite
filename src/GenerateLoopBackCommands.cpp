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
#include "SetToken.hpp"
#include <iostream>

namespace netcli {

  void CommandGenerator::generateLoopbacks(
      ConfigurationManager &mgr, std::set<std::string> &processedInterfaces) {
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

} // namespace netcli
