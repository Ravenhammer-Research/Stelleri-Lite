#include "ConfigurationManager.hpp"
#include "InterfaceConfig.hpp"
#include "InterfaceToken.hpp"
#include "Parser.hpp"
#include <iostream>

void netcli::Parser::executeDeleteInterface(const InterfaceToken &tok,
                                            ConfigurationManager *mgr) const {
  (void)mgr;

  const std::string name = tok.name();
  if (name.empty()) {
    std::cerr << "delete interface: missing interface name\n";
    return;
  }

  if (!InterfaceConfig::exists(name)) {
    std::cerr << "delete interface: interface '" << name << "' not found\n";
    return;
  }

  InterfaceConfig ic;
  ic.name = name;
  try {
    ic.destroy();
    std::cout << "delete interface: removed '" << name << "'\n";
  } catch (const std::exception &e) {
    std::cerr << "delete interface: failed to remove '" << name << "': "
              << e.what() << "\n";
  }
}
