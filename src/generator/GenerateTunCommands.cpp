#include "GenerateTunCommands.hpp"
#include "InterfaceToken.hpp"
#include "TunInterfaceConfig.hpp"
#include <iostream>

namespace netcli {

  void generateTunCommands(ConfigurationManager &mgr,
                           std::set<std::string> &processedInterfaces) {
    auto tuns = mgr.GetTunInterfaces(mgr.GetInterfaces());
    for (const auto &ifc : tuns) {
      if (processedInterfaces.count(ifc.name))
        continue;
      std::cout << std::string("set ") +
                       InterfaceToken::toString(
                           const_cast<TunInterfaceConfig *>(&ifc))
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
