#if !defined(STELLERI_NETCONF) || STELLERI_NETCONF != 1
#error "netconf headers are for the STELLERI_NETCONF build only"
#endif

#include "NetconfConfigurationManager.hpp"

std::vector<SixToFourInterfaceConfig>
NetconfConfigurationManager::GetSixToFourInterfaces(
    const std::vector<InterfaceConfig> & /*bases*/) const {
  return {};
}

void NetconfConfigurationManager::CreateSixToFour(
    const std::string & /*name*/) const {}

void NetconfConfigurationManager::SaveSixToFour(
    const SixToFourInterfaceConfig & /*t*/) const {}

void NetconfConfigurationManager::DestroySixToFour(
    const std::string & /*name*/) const {}
