/*
 * Copyright (c) 2026, Ravenhammer Research Inc.
 * All rights reserved.
 */

#include "GenerateArpCommands.hpp"
#include "ArpConfig.hpp"
#include "ArpToken.hpp"
#include <iostream>

namespace netcli {

  void generateArpCommands(ConfigurationManager &mgr) {
    auto entries = mgr.GetArpEntries();
    for (auto &entry : entries) {
      // Only care about published. 
      if (!entry.published)
        continue;

      std::cout << "set " << ArpToken::toString(&entry) << "\n";
    }
  }

} // namespace netcli
