/*
 * Copyright (c) 2026, Ravenhammer Research Inc.
 * All rights reserved.
 */

#include "CommandGenerator.hpp"
#include "InterfaceToken.hpp"
#include "InterfaceConfig.hpp"
#include <iostream>

namespace netcli {

void CommandGenerator::generateLaggs(ConfigurationManager &mgr,
                                     std::set<std::string> &processedInterfaces) {
  auto laggs = mgr.GetLaggInterfaces();
  for (const auto &ifc : laggs) {
    if (processedInterfaces.count(ifc.name))
      continue;

    std::cout << std::string("set ") + InterfaceToken::toString(const_cast<LaggConfig *>(&ifc)) << "\n";
    processedInterfaces.insert(ifc.name);

    for (const auto &alias : ifc.aliases) {
      InterfaceConfig tmp = ifc;
      tmp.address = alias->clone();
      std::cout << std::string("set ") + InterfaceToken::toString(&tmp) << "\n";
    }
  }
}

} // namespace netcli
