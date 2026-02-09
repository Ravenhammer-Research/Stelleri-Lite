/*
 * Copyright (c) 2026, Ravenhammer Research Inc.
 * All rights reserved.
 */

#include "CommandGenerator.hpp"
#include "InterfaceToken.hpp"
#include "InterfaceConfig.hpp"
#include <iostream>

namespace netcli {

void CommandGenerator::generateTunnels(ConfigurationManager &mgr,
                                       std::set<std::string> &processedInterfaces) {
  auto tunnels = mgr.GetTunnelInterfaces();
  for (const auto &ifc : tunnels) {
    if (processedInterfaces.count(ifc.name))
      continue;

    std::cout << std::string("set ") + InterfaceToken::toString(const_cast<TunnelConfig *>(&ifc)) << "\n";
    processedInterfaces.insert(ifc.name);

    for (const auto &alias : ifc.aliases) {
      InterfaceConfig tmp = ifc;
      tmp.address = alias->clone();
      std::cout << std::string("set ") + InterfaceToken::toString(&tmp) << "\n";
    }
  }
}

} // namespace netcli
