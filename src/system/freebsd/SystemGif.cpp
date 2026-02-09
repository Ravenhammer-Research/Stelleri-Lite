/*
 * GIF system helper implementations
 */

#include "Socket.hpp"
#include "SystemConfigurationManager.hpp"
#include "GifInterfaceConfig.hpp"

#include "IPAddress.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <net/if.h>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/ioctl.h>
#include <sys/socket.h>

void SystemConfigurationManager::SaveGif(const GifInterfaceConfig &t) const {
  if (t.name.empty())
    throw std::runtime_error("GifInterfaceConfig has no interface name set");

  if (!t.source || !t.destination) {
    throw std::runtime_error("Gif endpoints not configured");
  }

  if (!InterfaceConfig::exists(*this, t.name))
    CreateGif(t.name);
  else
    SaveInterface(static_cast<const InterfaceConfig &>(t));

  auto src_addr = IPAddress::fromString(t.source->toString());
  auto dst_addr = IPAddress::fromString(t.destination->toString());
  if (!src_addr || !dst_addr) {
    throw std::runtime_error("Invalid gif endpoint addresses");
  }
  if (src_addr->family() != dst_addr->family()) {
    throw std::runtime_error(
        "Gif endpoints must be same address family (both IPv4 or IPv6)");
  }

  Socket tsock(AF_INET, SOCK_DGRAM);

  struct ifaliasreq ifra;
  std::memset(&ifra, 0, sizeof(ifra));
  std::strncpy(ifra.ifra_name, t.name.c_str(), IFNAMSIZ - 1);

  if (src_addr->family() == AddressFamily::IPv4) {
    auto v4src = dynamic_cast<const IPv4Address *>(src_addr.get());
    auto v4dst = dynamic_cast<const IPv4Address *>(dst_addr.get());

    struct sockaddr_in *sin_src =
        reinterpret_cast<struct sockaddr_in *>(&ifra.ifra_addr);
    sin_src->sin_family = AF_INET;
    sin_src->sin_len = sizeof(struct sockaddr_in);
    sin_src->sin_addr.s_addr = htonl(v4src->value());

    struct sockaddr_in *sin_dst =
        reinterpret_cast<struct sockaddr_in *>(&ifra.ifra_broadaddr);
    sin_dst->sin_family = AF_INET;
    sin_dst->sin_len = sizeof(struct sockaddr_in);
    sin_dst->sin_addr.s_addr = htonl(v4dst->value());

    if (ioctl(tsock, SIOCSIFPHYADDR, &ifra) < 0) {
      throw std::runtime_error("Failed to configure GIF endpoints: " +
                               std::string(strerror(errno)));
    }
  } else {
    std::cerr << "Warning: IPv6 gif configuration requires routing socket - "
                 "not fully implemented\n";
  }
}

void SystemConfigurationManager::CreateGif(const std::string &nm) const {
  cloneInterface(nm, SIOCIFCREATE);
}

std::vector<GifInterfaceConfig> SystemConfigurationManager::GetGifInterfaces(
    const std::optional<VRFConfig> &vrf) const {
  auto bases = GetInterfaces(vrf);
  std::vector<GifInterfaceConfig> out;
  for (const auto &ic : bases) {
    if (ic.type == InterfaceType::Gif) {
      out.emplace_back(ic);
    }
  }
  return out;
}
