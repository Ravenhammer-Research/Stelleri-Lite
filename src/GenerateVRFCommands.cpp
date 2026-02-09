/*
 * Copyright (c) 2026, Ravenhammer Research Inc.
 * All rights reserved.
 */

#include "CommandGenerator.hpp"
#include "VRFToken.hpp"
#include <iostream>

namespace netcli {

void CommandGenerator::generateVRFs(ConfigurationManager &mgr) {
  auto vrfs = mgr.GetVrfs();
  for (const auto &v : vrfs) {
    std::cout << "set vrf fibnum " << v.table << "\n";
  }
}

} // namespace netcli
