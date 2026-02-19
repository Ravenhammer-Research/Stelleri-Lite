#pragma once

#include "ConfigurationManager.hpp"
#include <set>

namespace netcli {
  void generateTapCommands(ConfigurationManager &mgr,
                           std::set<std::string> &processedInterfaces);
}
