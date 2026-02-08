/*
 * TAP system helper implementations
 */

#include "SystemConfigurationManager.hpp"
#include "TapConfig.hpp"
#include "Socket.hpp"

#include <cerrno>
#include <cstring>
#include <net/if.h>
#include <stdexcept>
#include <sys/ioctl.h>
#include <sys/socket.h>

void SystemConfigurationManager::CreateTap(const std::string &name) const {
  if (InterfaceConfig::exists(name))
    return;
  Socket sock(AF_INET, SOCK_DGRAM);

  struct ifreq ifr;
  std::memset(&ifr, 0, sizeof(ifr));
  std::strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ - 1);

  if (ioctl(sock, SIOCIFCREATE, &ifr) < 0) {
    throw std::runtime_error("Failed to create interface '" + name + "': " +
                             std::string(strerror(errno)));
  }
}

void SystemConfigurationManager::SaveTap(const TapConfig &tap) const {
  if (tap.name.empty())
    throw std::runtime_error("TapConfig has no interface name set");

  if (!InterfaceConfig::exists(tap.name))
    CreateTap(tap.name);

  // Use generic interface save for addresses/mtu/flags
  SaveInterface(tap);
}
