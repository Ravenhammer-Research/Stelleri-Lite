#include "GenerateWlanCommands.hpp"
#include "InterfaceToken.hpp"
#include "WlanInterfaceConfig.hpp"
#include <iostream>

namespace netcli {

  void generateWlanCommands(ConfigurationManager &mgr,
                            std::set<std::string> &processedInterfaces) {
    auto wlans = mgr.GetWlanInterfaces(mgr.GetInterfaces());
    for (const auto &ifc : wlans) {
      if (processedInterfaces.count(ifc.name))
        continue;
      std::cout << std::string("set ") +
                       InterfaceToken::toString(
                           const_cast<WlanInterfaceConfig *>(&ifc))
                << "\n";
      processedInterfaces.insert(ifc.name);
      for (const auto &alias : ifc.aliases) {
        InterfaceConfig tmp = ifc;
        tmp.address = alias->clone();
        std::cout << std::string("set ") + InterfaceToken::toString(&tmp)
                  << "\n";
      }
    }
  }

} // namespace netcli
