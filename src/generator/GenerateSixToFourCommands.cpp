/*
 * Copyright (c) 2026, Ravenhammer Research Inc.
 * All rights reserved.
 */

#include "GenerateSixToFourCommands.hpp"
#include "InterfaceConfig.hpp"
#include "InterfaceToken.hpp"
#include "InterfaceType.hpp"
#include "SixToFourInterfaceConfig.hpp"
#include <iostream>

namespace netcli {

  void generateSixToFourCommands(ConfigurationManager &mgr,
                                 std::set<std::string> &processedInterfaces) {
    auto ifs = mgr.GetInterfaces();
    for (const auto &ifc : ifs) {
      if (ifc.type != InterfaceType::SixToFour)
        continue;
      if (processedInterfaces.count(ifc.name))
        continue;

      SixToFourInterfaceConfig sc(ifc);
      std::cout << "set " << InterfaceToken::toString(&sc) << "\n";
      processedInterfaces.insert(ifc.name);

      for (const auto &alias : ifc.aliases) {
        InterfaceConfig tmp = ifc;
        tmp.address = alias->clone();
        std::cout << "set " << InterfaceToken::toString(&tmp) << "\n";
      }
    }
  }

} // namespace netcli
