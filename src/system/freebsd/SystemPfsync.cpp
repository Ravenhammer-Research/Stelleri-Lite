/* Pfsync system helper implementations */

#include "PfsyncInterfaceConfig.hpp"
#include "SystemConfigurationManager.hpp"

#include <stdexcept>
#include <net/if.h>
#include <sys/sockio.h>

void SystemConfigurationManager::CreatePfsync(const std::string &name) const {
	if (InterfaceConfig::exists(*this, name))
		return;
	cloneInterface(name, SIOCIFCREATE);
}

void SystemConfigurationManager::SavePfsync(const PfsyncInterfaceConfig &p) const {
	if (p.name.empty())
		throw std::runtime_error("PfsyncInterfaceConfig has no interface name set");

	if (!InterfaceConfig::exists(*this, p.name))
		CreatePfsync(p.name);

	SaveInterface(p);
}

void SystemConfigurationManager::DestroyPfsync(const std::string &name) const {
	DestroyInterface(name);
}

std::vector<PfsyncInterfaceConfig> SystemConfigurationManager::GetPfsyncInterfaces(
		const std::vector<InterfaceConfig> &bases) const {
	std::vector<PfsyncInterfaceConfig> out;
	for (const auto &ic : bases) {
		if (ic.type != InterfaceType::Pfsync)
			continue;
		out.emplace_back(ic);
	}
	return out;
}
