#pragma once

#include "ConfigurationManager.hpp"
#include <set>

namespace netcli {
  void generateIpsecCommands(ConfigurationManager &mgr,
                             std::set<std::string> &processedInterfaces);
}
