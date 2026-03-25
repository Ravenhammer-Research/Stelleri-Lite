#if !defined(STELLERI_NETCONF) || STELLERI_NETCONF != 1
#error "netconf headers are for the STELLERI_NETCONF build only"
#endif

#include "NetconfConfigurationManager.hpp"

std::vector<PflogInterfaceConfig>
NetconfConfigurationManager::GetPflogInterfaces(
    const std::vector<InterfaceConfig> & /*bases*/) const {
  return {};
}

void NetconfConfigurationManager::CreatePflog(
    const std::string & /*name*/) const {}

void NetconfConfigurationManager::SavePflog(
    const PflogInterfaceConfig & /*p*/) const {}

void NetconfConfigurationManager::DestroyPflog(
    const std::string & /*name*/) const {}
