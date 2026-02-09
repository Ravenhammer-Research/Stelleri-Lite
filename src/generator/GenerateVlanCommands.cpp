/*
 * Copyright (c) 2026, Ravenhammer Research Inc.
 * All rights reserved.
 */

#include "CommandGenerator.hpp"
#include "InterfaceConfig.hpp"
#include "InterfaceToken.hpp"
#include <iostream>

namespace netcli {

  void
  CommandGenerator::generateVLANs(ConfigurationManager &mgr,
                                  std::set<std::string> &processedInterfaces) {
    auto vlans = mgr.GetVLANInterfaces();
    for (const auto &ifc : vlans) {
      if (processedInterfaces.count(ifc.name))
        continue;

      std::cout << std::string("set ") +
               InterfaceToken::toString(const_cast<VlanInterfaceConfig *>(&ifc))
                << "\n";
      processedInterfaces.insert(ifc.name);

      for (const auto &alias : ifc.aliases) {
        InterfaceConfig tmp = ifc;
        tmp.address = alias->clone();
        std::cout << std::string("set ") + InterfaceToken::toString(&tmp)
                  << "\n";
      }
    }
  }

} // namespace netcli
