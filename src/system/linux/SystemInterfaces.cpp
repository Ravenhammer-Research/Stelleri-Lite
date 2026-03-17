/*
 * Copyright (c) 2026, Ravenhammer Research Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "InterfaceConfig.hpp"
#include "Socket.hpp"
#include "SystemConfigurationManager.hpp"

#include <algorithm>
#include <cstring>
#include <ifaddrs.h>
#include <iostream>
#include <linux/ethtool.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>

namespace {

  InterfaceType probeInterfaceType(int sock, const std::string &name) {
    struct ifreq ifr{};
    std::strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ - 1);

    // 1. Try ethtool to get driver information
    struct ethtool_drvinfo drvinfo{};
    drvinfo.cmd = ETHTOOL_GDRVINFO;
    ifr.ifr_data = reinterpret_cast<char *>(&drvinfo);
    if (ioctl(sock, SIOCETHTOOL, &ifr) == 0) {
      std::string driver(drvinfo.driver);
      if (driver == "bridge")
        return InterfaceType::Bridge;
      if (driver == "bonding")
        return InterfaceType::Lagg;
      if (driver == "8021q")
        return InterfaceType::VLAN;
      if (driver == "vrf")
        return InterfaceType::VRF;
      if (driver == "tun")
        return InterfaceType::Tun;
      if (driver == "veth")
        return InterfaceType::Epair;
      if (driver == "vxlan")
        return InterfaceType::VXLAN;
      if (driver == "gre" || driver == "gretap")
        return InterfaceType::GRE;
      if (driver == "sit")
        return InterfaceType::SixToFour;
      if (driver == "ipip")
        return InterfaceType::Ipip;
      if (driver == "wireguard")
        return InterfaceType::WireGuard;
      if (driver == "macvlan")
        return InterfaceType::Ethernet; // Or a specific type if added
    }

    // 2. Try Wireless Extensions (SIOCGIWNAME = 0x8B01)
    struct ifreq iwr{};
    std::strncpy(iwr.ifr_name, name.c_str(), IFNAMSIZ - 1);
    if (ioctl(sock, 0x8B01, &iwr) == 0) {
      return InterfaceType::Wireless;
    }

    // 3. Fallback to hardware type
    std::memset(&ifr, 0, sizeof(ifr));
    std::strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ - 1);
    if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
      switch (ifr.ifr_hwaddr.sa_family) {
      case ARPHRD_LOOPBACK:
        return InterfaceType::Loopback;
      case ARPHRD_ETHER:
        return InterfaceType::Ethernet;
      case ARPHRD_PPP:
        return InterfaceType::PPP;
      case ARPHRD_TUNNEL:
      case ARPHRD_TUNNEL6:
        return InterfaceType::IPsec;
      case ARPHRD_SIT:
        return InterfaceType::SixToFour;
      case ARPHRD_IEEE80211:
        return InterfaceType::Wireless;
      }
    }

    return InterfaceType::Unknown;
  }

} // anonymous namespace

void SystemConfigurationManager::populateInterfaceMetadata(
    InterfaceConfig &ic) const {
  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0)
    return;

  struct ifreq ifr{};
  std::strncpy(ifr.ifr_name, ic.name.c_str(), IFNAMSIZ - 1);

  // Type probing
  ic.type = probeInterfaceType(sock, ic.name);

  // MTU
  if (ioctl(sock, SIOCGIFMTU, &ifr) == 0) {
    ic.mtu = ifr.ifr_mtu;
  }

  // HW address
  if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
    unsigned char *mac =
        reinterpret_cast<unsigned char *>(ifr.ifr_hwaddr.sa_data);
    char macbuf[32];
    std::snprintf(macbuf, sizeof(macbuf), "%02x:%02x:%02x:%02x:%02x:%02x",
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    if (std::string(macbuf) != "00:00:00:00:00:00")
      ic.hwaddr = std::string(macbuf);
  }

  // Index
  ic.index = if_nametoindex(ic.name.c_str());

  // Flags and Link State
  if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) {
    ic.flags = ifr.ifr_flags;
    ic.link_state = (ifr.ifr_flags & IFF_RUNNING) ? 1 : 0;
  }

  // Speed via Ethtool
  struct ethtool_cmd edata{};
  edata.cmd = ETHTOOL_GSET;
  ifr.ifr_data = reinterpret_cast<char *>(&edata);
  if (ioctl(sock, SIOCETHTOOL, &ifr) == 0) {
    uint32_t speed = ethtool_cmd_speed(&edata);
    if (speed != static_cast<uint32_t>(SPEED_UNKNOWN) && speed > 0) {
      ic.baudrate = static_cast<uint64_t>(speed) * 1000000;
    }
  }

  close(sock);
}

std::vector<InterfaceConfig> SystemConfigurationManager::GetInterfaces(
    const std::optional<VRFConfig> &vrf) const {
  std::vector<InterfaceConfig> results;
  struct ifaddrs *ifaddr, *ifa;

  if (getifaddrs(&ifaddr) == -1) {
    return results;
  }

  std::unordered_map<std::string, InterfaceConfig> iface_map;

  for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
    if (ifa->ifa_name == nullptr)
      continue;

    std::string name(ifa->ifa_name);
    auto [it, inserted] = iface_map.try_emplace(name);
    auto &ic = it->second;

    if (inserted) {
      ic.name = name;
      populateInterfaceMetadata(ic);
    }

    if (ifa->ifa_addr == nullptr)
      continue;

    if (ifa->ifa_addr->sa_family == AF_INET) {
      struct sockaddr_in *sa =
          reinterpret_cast<struct sockaddr_in *>(ifa->ifa_addr);
      struct sockaddr_in *nm =
          reinterpret_cast<struct sockaddr_in *>(ifa->ifa_netmask);
      int prefix = 0;
      if (nm) {
        uint32_t mask = ntohl(nm->sin_addr.s_addr);
        while (mask & 0x80000000) {
          prefix++;
          mask <<= 1;
        }
      }
      auto net =
          std::make_unique<IPv4Network>(ntohl(sa->sin_addr.s_addr), prefix);
      if (!ic.address) {
        ic.address = std::move(net);
      } else {
        ic.aliases.push_back(std::move(net));
      }
    } else if (ifa->ifa_addr->sa_family == AF_INET6) {
      struct sockaddr_in6 *sa6 =
          reinterpret_cast<struct sockaddr_in6 *>(ifa->ifa_addr);
      struct sockaddr_in6 *nm6 =
          reinterpret_cast<struct sockaddr_in6 *>(ifa->ifa_netmask);
      int prefix = 0;
      if (nm6) {
        for (int i = 0; i < 16; i++) {
          uint8_t byte = nm6->sin6_addr.s6_addr[i];
          while (byte & 0x80) {
            prefix++;
            byte <<= 1;
          }
        }
      }
      unsigned __int128 val = 0;
      for (int i = 0; i < 16; i++) {
        val = (val << 8) | sa6->sin6_addr.s6_addr[i];
      }
      auto net = std::make_unique<IPv6Network>(val, prefix);
      if (!ic.address) {
        ic.address = std::move(net);
      } else {
        ic.aliases.push_back(std::move(net));
      }
    }
  }

  freeifaddrs(ifaddr);

  for (auto &pair : iface_map) {
    if (matches_vrf(pair.second, vrf)) {
      results.push_back(std::move(pair.second));
    }
  }

  return results;
}

std::vector<InterfaceConfig> SystemConfigurationManager::GetInterfacesByGroup(
    const std::optional<VRFConfig> &vrf,
    std::string_view group [[maybe_unused]]) const {
  return GetInterfaces(vrf);
}

bool SystemConfigurationManager::InterfaceExists(std::string_view name) const {
  return if_nametoindex(std::string(name).c_str()) != 0;
}

bool SystemConfigurationManager::matches_vrf(const InterfaceConfig &ic
                                             [[maybe_unused]],
                                             const std::optional<VRFConfig> &vrf
                                             [[maybe_unused]]) const {
  if (!vrf)
    return true;
  if (!ic.vrf)
    return vrf->table == 0;
  return ic.vrf->table == vrf->table;
}

void SystemConfigurationManager::CreateInterface(const std::string &name
                                                 [[maybe_unused]]) const {}

void SystemConfigurationManager::DestroyInterface(const std::string &name
                                                  [[maybe_unused]]) const {}

void SystemConfigurationManager::SaveInterface(const InterfaceConfig &ic
                                               [[maybe_unused]]) const {}

void SystemConfigurationManager::RemoveInterfaceAddress(
    const std::string &ifname [[maybe_unused]],
    const std::string &addr [[maybe_unused]]) const {}

void SystemConfigurationManager::RemoveInterfaceGroup(const std::string &ifname
                                                      [[maybe_unused]],
                                                      const std::string &group
                                                      [[maybe_unused]]) const {}

std::vector<std::string>
SystemConfigurationManager::GetInterfaceAddresses(const std::string &ifname,
                                                  int family) const {
  std::vector<std::string> addresses;
  struct ifaddrs *ifaddr, *ifa;

  if (getifaddrs(&ifaddr) == -1) {
    return addresses;
  }

  for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
    if (ifa->ifa_name == nullptr || ifname != ifa->ifa_name)
      continue;
    if (ifa->ifa_addr == nullptr || ifa->ifa_addr->sa_family != family)
      continue;

    char host[NI_MAXHOST];
    int s = getnameinfo(ifa->ifa_addr,
                        (family == AF_INET) ? sizeof(struct sockaddr_in)
                                            : sizeof(struct sockaddr_in6),
                        host, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST);
    if (s == 0) {
      addresses.push_back(host);
    }
  }

  freeifaddrs(ifaddr);
  return addresses;
}
