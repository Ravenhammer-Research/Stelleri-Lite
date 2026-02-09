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

#include "ConfigurationManager.hpp"
#include "IPNetwork.hpp"
#include "InterfaceConfig.hpp"
#include "InterfaceToken.hpp"
#include <iostream>

namespace netcli {

  void executeDeleteInterface(const InterfaceToken &tok,
                              ConfigurationManager *mgr) {
    const std::string name = tok.name();
    if (name.empty()) {
      std::cerr << "delete interface: missing interface name\n";
      return;
    }

    if (!InterfaceConfig::exists(*mgr, name)) {
      std::cerr << "delete interface: interface '" << name << "' not found\n";
      return;
    }

    InterfaceConfig ic;
    ic.name = name;
    try {
      // If token requests group deletion, remove just the group
      if (tok.group) {
        mgr->RemoveInterfaceGroup(name, *tok.group);
        std::cout << "delete interface: removed group '" << *tok.group
                  << "' from '" << name << "'\n";
        return;
      }

      // If token requests address-level deletion (inet/inet6), handle
      // targeted removal rather than destroying the entire interface.
      if (tok.address || tok.address_family) {
        std::vector<std::string> to_remove;
        if (tok.address) {
          to_remove.push_back(*tok.address);
        } else if (tok.address_family && *tok.address_family == AF_INET) {
          to_remove = mgr->GetInterfaceAddresses(name, AF_INET);
        } else if (tok.address_family && *tok.address_family == AF_INET6) {
          to_remove = mgr->GetInterfaceAddresses(name, AF_INET6);
        }

        for (const auto &a : to_remove) {
          try {
            ic.removeAddress(*mgr, a);
            std::cout << "delete interface: removed address '" << a
                      << "' from '" << name << "'\n";
          } catch (const std::exception &e) {
            std::cerr << "delete interface: failed to remove address '" << a
                      << "': " << e.what() << "\n";
          }
        }
        return;
      }

      // Default: destroy the interface
      ic.destroy(*mgr);
      std::cout << "delete interface: removed '" << name << "'\n";
    } catch (const std::exception &e) {
      std::cerr << "delete interface: failed to remove '" << name
                << "': " << e.what() << "\n";
    }
  }
} // namespace netcli
