#include "GenerateGifCommands.hpp"
#include "GifInterfaceConfig.hpp"
#include "InterfaceToken.hpp"
#include <iostream>

namespace netcli {

  void generateGifCommands(ConfigurationManager &mgr,
                           std::set<std::string> &processedInterfaces) {
    auto gifs = mgr.GetGifInterfaces(mgr.GetInterfaces());
    for (const auto &ifc : gifs) {
      if (processedInterfaces.count(ifc.name))
        continue;
      std::cout << std::string("set ") +
                       InterfaceToken::toString(
                           const_cast<GifInterfaceConfig *>(&ifc))
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
