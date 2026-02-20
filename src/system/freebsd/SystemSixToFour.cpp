/* 6to4 system helper implementations */

#include "SixToFourInterfaceConfig.hpp"
#include "SystemConfigurationManager.hpp"

#include <stdexcept>
#include <net/if.h>
#include <sys/sockio.h>

void SystemConfigurationManager::CreateSixToFour(const std::string &name) const {
	if (InterfaceConfig::exists(*this, name))
		return;
	cloneInterface(name, SIOCIFCREATE);
}

void SystemConfigurationManager::SaveSixToFour(
		const SixToFourInterfaceConfig &t) const {
	if (t.name.empty())
		throw std::runtime_error("SixToFourInterfaceConfig has no interface name set");

	if (!InterfaceConfig::exists(*this, t.name))
		CreateSixToFour(t.name);

	// Use generic interface save for addresses/mtu/flags
	SaveInterface(t);
}

void SystemConfigurationManager::DestroySixToFour(const std::string &name) const {
	DestroyInterface(name);
}

std::vector<SixToFourInterfaceConfig>
SystemConfigurationManager::GetSixToFourInterfaces(
		const std::vector<InterfaceConfig> &bases) const {
	std::vector<SixToFourInterfaceConfig> out;
	for (const auto &ic : bases) {
		if (ic.type == InterfaceType::SixToFour) {
			out.emplace_back(ic);
		}
	}
	return out;
}
