#include "GenerateCarpCommands.hpp"
#include "CarpInterfaceConfig.hpp"
#include "InterfaceToken.hpp"
#include <iostream>

namespace netcli {

  void generateCarpCommands(ConfigurationManager &mgr,
                            std::set<std::string> &processedInterfaces) {
    auto carps = mgr.GetCarpInterfaces(mgr.GetInterfaces());
    for (const auto &ifc : carps) {
      if (processedInterfaces.count(ifc.name))
        continue;
      std::cout << std::string("set ") +
                       InterfaceToken::toString(
                           const_cast<CarpInterfaceConfig *>(&ifc))
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
