/* Pflog system helper implementations */

#include "PflogInterfaceConfig.hpp"
#include "SystemConfigurationManager.hpp"

#include <stdexcept>
#include <net/if.h>
#include <sys/sockio.h>

void SystemConfigurationManager::CreatePflog(const std::string &name) const {
	if (InterfaceConfig::exists(*this, name))
		return;
	cloneInterface(name, SIOCIFCREATE);
}

void SystemConfigurationManager::SavePflog(const PflogInterfaceConfig &p) const {
	if (p.name.empty())
		throw std::runtime_error("PflogInterfaceConfig has no interface name set");

	if (!InterfaceConfig::exists(*this, p.name))
		CreatePflog(p.name);

	SaveInterface(p);
}

void SystemConfigurationManager::DestroyPflog(const std::string &name) const {
	DestroyInterface(name);
}

std::vector<PflogInterfaceConfig> SystemConfigurationManager::GetPflogInterfaces(
		const std::vector<InterfaceConfig> &bases) const {
	std::vector<PflogInterfaceConfig> out;
	for (const auto &ic : bases) {
		if (ic.type != InterfaceType::Pflog)
			continue;
		out.emplace_back(ic);
	}
	return out;
}
