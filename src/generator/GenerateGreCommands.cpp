#include "GenerateGreCommands.hpp"
#include "GreInterfaceConfig.hpp"
#include "InterfaceToken.hpp"
#include <iostream>

namespace netcli {

  void generateGreCommands(ConfigurationManager &mgr,
                           std::set<std::string> &processedInterfaces) {
    auto gres = mgr.GetGreInterfaces(mgr.GetInterfaces());
    for (const auto &ifc : gres) {
      if (processedInterfaces.count(ifc.name))
        continue;
      std::cout << std::string("set ") +
                       InterfaceToken::toString(
                           const_cast<GreInterfaceConfig *>(&ifc))
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
