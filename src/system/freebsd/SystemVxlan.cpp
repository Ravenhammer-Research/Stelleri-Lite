/*
 * VXLAN system helper implementations
 */

#include "SystemConfigurationManager.hpp"
#include "VxlanInterfaceConfig.hpp"

#include <net/if.h>
#include <sys/sockio.h>
#include <stdexcept>

void SystemConfigurationManager::CreateVxlan(const std::string &name) const {
  if (InterfaceConfig::exists(*this, name))
    return;
  cloneInterface(name, SIOCIFCREATE);
}

void SystemConfigurationManager::SaveVxlan(
    const VxlanInterfaceConfig &vxlan) const {
  if (vxlan.name.empty())
    throw std::runtime_error("VxlanInterfaceConfig has no interface name set");

  if (!InterfaceConfig::exists(*this, vxlan.name))
    CreateVxlan(vxlan.name);

  // Apply generic interface settings (addresses, MTU, flags).
  // VXLAN-specific parameters (VNI, local/remote addresses, ports) require
  // ifdrv-based ioctls which are done at interface creation time on FreeBSD.
  SaveInterface(vxlan);
}

std::vector<VxlanInterfaceConfig>
SystemConfigurationManager::GetVxlanInterfaces(
    const std::vector<InterfaceConfig> &bases) const {
  std::vector<VxlanInterfaceConfig> out;
  for (const auto &ic : bases) {
    if (ic.type == InterfaceType::VXLAN) {
      out.emplace_back(ic);
    }
  }
  return out;
}
