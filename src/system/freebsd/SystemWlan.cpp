/*
 * WLAN system helper implementations
 */

#include "SystemConfigurationManager.hpp"
#include "WlanInterfaceConfig.hpp"

#include <net/if.h>
#include <sys/sockio.h>

void SystemConfigurationManager::CreateWlan(const std::string &name) const {
  if (InterfaceConfig::exists(*this, name))
    return;
  cloneInterface(name, SIOCIFCREATE);
}

void SystemConfigurationManager::SaveWlan(const WlanInterfaceConfig &wlan) const {
  if (wlan.name.empty())
    throw std::runtime_error("WlanInterfaceConfig has no interface name set");

  if (!InterfaceConfig::exists(*this, wlan.name))
    CreateWlan(wlan.name);

  // Reuse generic interface save for addresses/mtu/flags
  SaveInterface(wlan);
}
