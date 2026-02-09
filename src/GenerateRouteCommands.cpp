/*
 * Copyright (c) 2026, Ravenhammer Research Inc.
 * All rights reserved.
 */

#include "CommandGenerator.hpp"
#include "RouteToken.hpp"
#include "RouteConfig.hpp"
#include <iostream>

namespace netcli {

void CommandGenerator::generateRoutes(ConfigurationManager &mgr) {
  auto routes = mgr.GetRoutes();
  for (const auto &r : routes) {
    std::cout << std::string("set ") + RouteToken::toString(const_cast<RouteConfig *>(&r)) << "\n";
  }
}

void CommandGenerator::generateVirtuals(ConfigurationManager &mgr,
                                        std::set<std::string> &processedInterfaces) {
  auto virtuals = mgr.GetVirtualInterfaces();
  for (const auto &ifc : virtuals) {
    bool is_epair = false;
    for (const auto &g : ifc.groups) if (g == "epair") { is_epair = true; break; }
    if (is_epair)
      continue; // epair handled elsewhere

    if (ifc.address || !ifc.aliases.empty()) {
      std::cout << std::string("set ") + InterfaceToken::toString(const_cast<VirtualInterfaceConfig *>(&ifc)) << "\n";
      processedInterfaces.insert(ifc.name);
      for (const auto &alias : ifc.aliases) {
        InterfaceConfig tmp = ifc;
        tmp.address = alias->clone();
        std::cout << std::string("set ") + InterfaceToken::toString(&tmp) << "\n";
      }
    }
  }
}

} // namespace netcli
