/*
 * Copyright (c) 2026, Ravenhammer Research Inc.
 * All rights reserved.
 */

#include "GeneratePfsyncCommands.hpp"
#include "InterfaceConfig.hpp"
#include "InterfaceToken.hpp"
#include "InterfaceType.hpp"
#include <iostream>

namespace netcli {

  void generatePfsyncCommands(ConfigurationManager &mgr,
                              std::set<std::string> &processedInterfaces) {
    auto ifs = mgr.GetInterfaces();
    for (const auto &ifc : ifs) {
      if (ifc.type != InterfaceType::Pfsync)
        continue;
      if (processedInterfaces.count(ifc.name))
        continue;

      std::cout << "set "
                << InterfaceToken::toString(const_cast<InterfaceConfig *>(&ifc))
                << "\n";
      processedInterfaces.insert(ifc.name);

      for (const auto &alias : ifc.aliases) {
        InterfaceConfig tmp = ifc;
        tmp.address = alias->clone();
        std::cout << "set " << InterfaceToken::toString(&tmp) << "\n";
      }
    }
  }

} // namespace netcli
