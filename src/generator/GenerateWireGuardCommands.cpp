/*
 * Copyright (c) 2026, Ravenhammer Research Inc.
 * All rights reserved.
 */

#include "GenerateWireGuardCommands.hpp"
#include "InterfaceConfig.hpp"
#include "InterfaceToken.hpp"
#include "InterfaceType.hpp"
#include "WireGuardInterfaceConfig.hpp"
#include <iostream>

namespace netcli {

  void generateWireGuardCommands(ConfigurationManager &mgr,
                                 std::set<std::string> &processedInterfaces) {
    auto ifs = mgr.GetInterfaces();
    for (const auto &ifc : ifs) {
      if (ifc.type != InterfaceType::WireGuard)
        continue;
      if (processedInterfaces.count(ifc.name))
        continue;

      WireGuardInterfaceConfig wgc(ifc);
      std::cout << "set " << InterfaceToken::toString(&wgc) << "\n";
      processedInterfaces.insert(ifc.name);

      for (const auto &alias : ifc.aliases) {
        InterfaceConfig tmp = ifc;
        tmp.address = alias->clone();
        std::cout << "set " << InterfaceToken::toString(&tmp) << "\n";
      }
    }
  }

} // namespace netcli
