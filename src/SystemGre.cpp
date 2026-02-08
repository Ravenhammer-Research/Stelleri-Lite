/*
 * GRE system helper implementations
 */

#include "GREConfig.hpp"
#include "Socket.hpp"
#include "SystemConfigurationManager.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <net/if.h>
#include <net/if_gre.h>
#include <stdexcept>
#include <sys/ioctl.h>
#include <sys/socket.h>

void SystemConfigurationManager::CreateGre(const std::string &name) const {
  if (InterfaceConfig::exists(*this, name))
    return;
  cloneInterface(name, SIOCIFCREATE);
}

void SystemConfigurationManager::SaveGre(const GREConfig &gre) const {
  if (gre.name.empty())
    throw std::runtime_error("GREConfig has no interface name set");

  if (!InterfaceConfig::exists(*this, gre.name))
    CreateGre(gre.name);

  // Apply generic interface settings (addresses, MTU, flags)
  SaveInterface(gre);

  Socket sock(AF_INET, SOCK_DGRAM);

  // Set GRE key if specified
  if (gre.greKey) {
    struct ifreq ifr;
    prepare_ifreq(ifr, gre.name);
    ifr.ifr_data =
        reinterpret_cast<caddr_t>(const_cast<uint32_t *>(&*gre.greKey));
    if (ioctl(sock, GRESKEY, &ifr) < 0) {
      throw std::runtime_error("GRESKEY failed for " + gre.name + ": " +
                               strerror(errno));
    }
  }

  // Set GRE options if specified
  if (gre.greOptions) {
    struct ifreq ifr;
    prepare_ifreq(ifr, gre.name);
    ifr.ifr_data =
        reinterpret_cast<caddr_t>(const_cast<uint32_t *>(&*gre.greOptions));
    if (ioctl(sock, GRESOPTS, &ifr) < 0) {
      throw std::runtime_error("GRESOPTS failed for " + gre.name + ": " +
                               strerror(errno));
    }
  }

  // Set source address if specified
  if (gre.greSource) {
    struct ifreq ifr;
    prepare_ifreq(ifr, gre.name);
    struct sockaddr_in *sin =
        reinterpret_cast<struct sockaddr_in *>(&ifr.ifr_addr);
    sin->sin_family = AF_INET;
    sin->sin_len = sizeof(struct sockaddr_in);
    if (inet_pton(AF_INET, gre.greSource->c_str(), &sin->sin_addr) != 1) {
      throw std::runtime_error("Invalid GRE source address: " + *gre.greSource);
    }
    if (ioctl(sock, GRESADDRS, &ifr) < 0) {
      throw std::runtime_error("GRESADDRS failed for " + gre.name + ": " +
                               strerror(errno));
    }
  }

  // Set destination address if specified
  if (gre.greDestination) {
    struct ifreq ifr;
    prepare_ifreq(ifr, gre.name);
    struct sockaddr_in *sin =
        reinterpret_cast<struct sockaddr_in *>(&ifr.ifr_addr);
    sin->sin_family = AF_INET;
    sin->sin_len = sizeof(struct sockaddr_in);
    if (inet_pton(AF_INET, gre.greDestination->c_str(), &sin->sin_addr) != 1) {
      throw std::runtime_error("Invalid GRE destination address: " +
                               *gre.greDestination);
    }
    if (ioctl(sock, GRESADDRD, &ifr) < 0) {
      throw std::runtime_error("GRESADDRD failed for " + gre.name + ": " +
                               strerror(errno));
    }
  }
}

std::vector<GREConfig> SystemConfigurationManager::GetGreInterfaces(
    const std::optional<VRFConfig> &vrf) const {
  auto bases = GetInterfaces(vrf);
  std::vector<GREConfig> out;
  for (const auto &ic : bases) {
    if (ic.type == InterfaceType::GRE || ic.name.rfind("gre", 0) == 0) {
      GREConfig gc(ic);

      // Query GRE key
      Socket sock(AF_INET, SOCK_DGRAM);
      struct ifreq ifr;
      prepare_ifreq(ifr, ic.name);
      uint32_t key = 0;
      ifr.ifr_data = reinterpret_cast<caddr_t>(&key);
      if (ioctl(sock, GREGKEY, &ifr) == 0 && key != 0)
        gc.greKey = key;

      // Query GRE options
      uint32_t opts = 0;
      prepare_ifreq(ifr, ic.name);
      ifr.ifr_data = reinterpret_cast<caddr_t>(&opts);
      if (ioctl(sock, GREGOPTS, &ifr) == 0)
        gc.greOptions = opts;

      out.push_back(std::move(gc));
    }
  }
  return out;
}
