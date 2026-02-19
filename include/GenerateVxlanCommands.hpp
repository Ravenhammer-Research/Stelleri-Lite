#pragma once

#include "ConfigurationManager.hpp"
#include <set>

namespace netcli {
  void generateVxlanCommands(ConfigurationManager &mgr,
                             std::set<std::string> &processedInterfaces);
}
