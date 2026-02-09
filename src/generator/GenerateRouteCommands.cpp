/*
 * Copyright (c) 2026, Ravenhammer Research Inc.
 * All rights reserved.
 */

#include "CommandGenerator.hpp"
#include "RouteConfig.hpp"
#include "RouteToken.hpp"
#include <iostream>

namespace netcli {

  void CommandGenerator::generateRoutes(ConfigurationManager &mgr) {
    auto routes = mgr.GetRoutes();
    for (const auto &r : routes) {
      std::cout << std::string("set ") +
                       RouteToken::toString(const_cast<RouteConfig *>(&r))
                << "\n";
    }
  }  

} // namespace netcli
