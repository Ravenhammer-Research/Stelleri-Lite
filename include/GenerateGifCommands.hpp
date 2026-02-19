#pragma once

#include "ConfigurationManager.hpp"
#include <set>

namespace netcli {
  void generateGifCommands(ConfigurationManager &mgr,
                           std::set<std::string> &processedInterfaces);
}
