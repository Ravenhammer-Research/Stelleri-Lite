#include "GenerateOvpnCommands.hpp"
#include "InterfaceToken.hpp"
#include "OvpnInterfaceConfig.hpp"
#include <iostream>

namespace netcli {

  void generateOvpnCommands(ConfigurationManager &mgr,
                            std::set<std::string> &processedInterfaces) {
    auto ovpns = mgr.GetOvpnInterfaces(mgr.GetInterfaces());
    for (const auto &ifc : ovpns) {
      if (processedInterfaces.count(ifc.name))
        continue;
      std::cout << std::string("set ") +
                       InterfaceToken::toString(
                           const_cast<OvpnInterfaceConfig *>(&ifc))
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
