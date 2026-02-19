#pragma once

#include "ConfigurationManager.hpp"
#include <set>

namespace netcli {
  void generateGreCommands(ConfigurationManager &mgr,
                           std::set<std::string> &processedInterfaces);
}
