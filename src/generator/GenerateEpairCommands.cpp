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
  CommandGenerator::generateEpairs(ConfigurationManager &mgr,
                                   std::set<std::string> &processedInterfaces) {
    auto virtuals = mgr.GetEpairInterfaces();
    for (const auto &ifc : virtuals) {
      if (processedInterfaces.count(ifc.name))
        continue;

      bool is_epair = false;
      for (const auto &g : ifc.groups) {
        if (g == "epair") {
          is_epair = true;
          break;
        }
      }
      if (!is_epair)
        continue;

      std::cout << std::string("set ") +
                       InterfaceToken::toString(
                           const_cast<VirtualInterfaceConfig *>(&ifc))
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
