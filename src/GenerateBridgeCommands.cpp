/*
 * Copyright (c) 2026, Ravenhammer Research Inc.
 * All rights reserved.
 */

#include "CommandGenerator.hpp"
#include "InterfaceToken.hpp"
#include "InterfaceConfig.hpp"
#include <iostream>

namespace netcli {

void CommandGenerator::generateBridges(ConfigurationManager &mgr,
                                      std::set<std::string> &processedInterfaces) {
  auto bridges = mgr.GetBridgeInterfaces();
  for (const auto &ifc : bridges) {
    if (processedInterfaces.count(ifc.name))
      continue;

    std::cout << std::string("set ") + InterfaceToken::toString(const_cast<BridgeInterfaceConfig *>(&ifc)) << "\n";
    processedInterfaces.insert(ifc.name);

    // Output aliases as separate commands
    for (const auto &alias : ifc.aliases) {
      InterfaceConfig tmp = ifc;
      tmp.address = alias->clone();
      std::cout << std::string("set ") + InterfaceToken::toString(&tmp) << "\n";
    }
  }
}

} // namespace netcli
