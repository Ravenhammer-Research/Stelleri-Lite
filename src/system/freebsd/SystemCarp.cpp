/*
 * CARP system helper implementations
 */

#include "CarpInterfaceConfig.hpp"
#include "Socket.hpp"
#include "SystemConfigurationManager.hpp"

#include <cerrno>
#include <cstring>
#include <net/if.h>
#include <netinet/ip_carp.h>
#include <stdexcept>
#include <sys/ioctl.h>

void SystemConfigurationManager::SaveCarp(
    const CarpInterfaceConfig &carp) const {
  if (carp.name.empty())
    throw std::runtime_error("CarpInterfaceConfig has no interface name set");

  if (!InterfaceConfig::exists(*this, carp.name))
    CreateInterface(carp.name);

  // Apply generic interface settings first (addresses, MTU, flags)
  SaveInterface(carp);

  // Apply CARP-specific parameters if any are set
  if (!carp.vhid && !carp.advskew && !carp.advbase && !carp.key)
    return;

  Socket sock(AF_INET, SOCK_DGRAM);

  // Read current CARP config first
  struct ifreq ifr;
  prepare_ifreq(ifr, carp.name);
  struct carpreq carpr;
  std::memset(&carpr, 0, sizeof(carpr));
  ifr.ifr_data = reinterpret_cast<caddr_t>(&carpr);

  // Try to read existing state; failure is OK for new interfaces
  (void)ioctl(sock, SIOCGVH, &ifr);

  if (carp.vhid)
    carpr.carpr_vhid = *carp.vhid;
  if (carp.advskew)
    carpr.carpr_advskew = *carp.advskew;
  if (carp.advbase)
    carpr.carpr_advbase = *carp.advbase;
  if (carp.key) {
    std::memset(carpr.carpr_key, 0, sizeof(carpr.carpr_key));
    std::size_t len = carp.key->size();
    if (len > sizeof(carpr.carpr_key))
      len = sizeof(carpr.carpr_key);
    std::memcpy(carpr.carpr_key, carp.key->data(), len);
  }

  prepare_ifreq(ifr, carp.name);
  ifr.ifr_data = reinterpret_cast<caddr_t>(&carpr);
  if (ioctl(sock, SIOCSVH, &ifr) < 0) {
    throw std::runtime_error("SIOCSVH failed for " + carp.name + ": " +
                             strerror(errno));
  }
}

std::vector<CarpInterfaceConfig> SystemConfigurationManager::GetCarpInterfaces(
    const std::vector<InterfaceConfig> &bases) const {
  std::vector<CarpInterfaceConfig> out;
  for (const auto &ic : bases) {
    if (ic.type == InterfaceType::Carp) {
      CarpInterfaceConfig cc(ic);

      Socket sock(AF_INET, SOCK_DGRAM);
      struct ifreq ifr;
      prepare_ifreq(ifr, ic.name);
      struct carpreq carpr;
      std::memset(&carpr, 0, sizeof(carpr));
      ifr.ifr_data = reinterpret_cast<caddr_t>(&carpr);

      if (ioctl(sock, SIOCGVH, &ifr) == 0) {
        if (carpr.carpr_vhid != 0)
          cc.vhid = carpr.carpr_vhid;
        cc.advskew = carpr.carpr_advskew;
        cc.advbase = carpr.carpr_advbase;
        switch (carpr.carpr_state) {
        case 0:
          cc.state = "INIT";
          break;
        case 1:
          cc.state = "BACKUP";
          break;
        case 2:
          cc.state = "MASTER";
          break;
        default:
          cc.state = std::to_string(carpr.carpr_state);
          break;
        }
      }

      out.push_back(std::move(cc));
    }
  }
  return out;
}
