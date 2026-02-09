/*
 * Copyright (c) 2026, Ravenhammer Research Inc.
 * All rights reserved.
 */

#include "CommandGenerator.hpp"
#include "InterfaceConfig.hpp"
#include "InterfaceToken.hpp"
#include <iostream>

namespace netcli {

  void CommandGenerator::generateBasicInterfaces(
      ConfigurationManager &mgr, std::set<std::string> &processedInterfaces) {
    auto interfaces = mgr.GetInterfaces();
    for (const auto &ifc : interfaces) {
      if (processedInterfaces.count(ifc.name))
        continue;
      if (ifc.type != InterfaceType::Ethernet)
        continue;

      std::cout << std::string("set ") +
                       InterfaceToken::toString(
                           const_cast<InterfaceConfig *>(&ifc))
                << "\n";
      processedInterfaces.insert(ifc.name);
    }

    // Assign addresses (only Ethernet)
    for (const auto &ifc : interfaces) {
      if (ifc.type != InterfaceType::Ethernet)
        continue;
      if (ifc.address) {
        std::cout << std::string("set ") +
                         InterfaceToken::toString(
                             const_cast<InterfaceConfig *>(&ifc))
                  << "\n";
      }
      for (const auto &alias : ifc.aliases) {
        InterfaceConfig tmp = ifc;
        tmp.address = alias->clone();
        std::cout << std::string("set ") + InterfaceToken::toString(&tmp)
                  << "\n";
      }
    }
  }

} // namespace netcli
