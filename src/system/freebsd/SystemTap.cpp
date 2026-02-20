/*
 * TAP system helper implementations
 */

#include "SystemConfigurationManager.hpp"
#include "TapInterfaceConfig.hpp"

#include <net/if.h>
#include <sys/sockio.h>
#include <stdexcept>

void SystemConfigurationManager::CreateTap(const std::string &name) const {
  if (InterfaceConfig::exists(*this, name))
    return;
  cloneInterface(name, SIOCIFCREATE);
}

void SystemConfigurationManager::SaveTap(const TapInterfaceConfig &tap) const {
  if (tap.name.empty())
    throw std::runtime_error("TapInterfaceConfig has no interface name set");

  if (!InterfaceConfig::exists(*this, tap.name))
    CreateTap(tap.name);

  // Use generic interface save for addresses/mtu/flags
  SaveInterface(tap);
}
