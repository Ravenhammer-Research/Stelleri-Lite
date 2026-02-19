#include "GenerateVxlanCommands.hpp"
#include "InterfaceToken.hpp"
#include "VxlanInterfaceConfig.hpp"
#include <iostream>

namespace netcli {

  void generateVxlanCommands(ConfigurationManager &mgr,
                             std::set<std::string> &processedInterfaces) {
    auto vxlans = mgr.GetVxlanInterfaces(mgr.GetInterfaces());
    for (const auto &ifc : vxlans) {
      if (processedInterfaces.count(ifc.name))
        continue;
      std::cout << std::string("set ") +
                       InterfaceToken::toString(
                           const_cast<VxlanInterfaceConfig *>(&ifc))
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
