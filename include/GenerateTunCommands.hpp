#pragma once

#include "ConfigurationManager.hpp"
#include <set>

namespace netcli {
  void generateTunCommands(ConfigurationManager &mgr,
                           std::set<std::string> &processedInterfaces);
}
