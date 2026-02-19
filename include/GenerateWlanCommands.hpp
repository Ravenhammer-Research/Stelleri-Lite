#pragma once

#include "ConfigurationManager.hpp"
#include <set>

namespace netcli {
  void generateWlanCommands(ConfigurationManager &mgr,
                            std::set<std::string> &processedInterfaces);
}
