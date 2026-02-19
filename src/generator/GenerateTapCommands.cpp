#include "GenerateTapCommands.hpp"
#include "InterfaceToken.hpp"
#include "TapInterfaceConfig.hpp"
#include <iostream>

namespace netcli {

  void generateTapCommands(ConfigurationManager &mgr,
                           std::set<std::string> &processedInterfaces) {
    // Tap interfaces use the basic InterfaceConfig toString since they
    // have no extra fields beyond the base class.
    auto bases = mgr.GetInterfaces();
    for (const auto &ifc : bases) {
      if (ifc.type != InterfaceType::Tap)
        continue;
      if (processedInterfaces.count(ifc.name))
        continue;
      TapInterfaceConfig tc(ifc);
      std::cout << std::string("set ") + InterfaceToken::toString(&tc) << "\n";
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
