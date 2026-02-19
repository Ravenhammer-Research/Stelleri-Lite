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
#include "WlanInterfaceConfig.hpp"

#include <algorithm>
#include <cstring>
#include <ifaddrs.h>
#include <iostream>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_types.h>
#include <netinet/in.h>
#include <netinet6/in6_var.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <unistd.h>
#include <unordered_map>

namespace {

  /// Map an ifaddrs entry to an InterfaceType using link-layer information.
  /// This is FreeBSD-specific (uses sockaddr_dl, IFT_* constants).
  InterfaceType ifAddrToInterfaceType(const struct ifaddrs *ifa) {
    if (!ifa)
      return InterfaceType::Unknown;
    unsigned int flags = ifa->ifa_flags;

    if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_LINK) {
      auto *sdl = reinterpret_cast<struct sockaddr_dl *>(ifa->ifa_addr);
      switch (sdl->sdl_type) {
      case IFT_ETHER:
      case IFT_FASTETHER:
      case IFT_GIGABITETHERNET:
      case IFT_FIBRECHANNEL:
      case IFT_AFLANE8023:
        return InterfaceType::Ethernet;
      case IFT_IEEE8023ADLAG:
        return InterfaceType::Lagg;
      case IFT_LOOP:
        return InterfaceType::Loopback;
      case IFT_PPP:
        return InterfaceType::PPP;
      case IFT_TUNNEL:
        return InterfaceType::Tunnel;
      case IFT_GIF:
        return InterfaceType::Gif;
      case IFT_FDDI:
        return InterfaceType::FDDI;
      case IFT_ISO88025:
      case IFT_ISO88023:
      case IFT_ISO88024:
      case IFT_ISO88026:
        return InterfaceType::TokenRing;
      case IFT_IEEE80211:
        return InterfaceType::Wireless;
      case IFT_BRIDGE:
        return InterfaceType::Bridge;
      case IFT_L2VLAN:
        return InterfaceType::VLAN;
      case IFT_ATM:
        return InterfaceType::ATM;
      case IFT_PROPVIRTUAL:
      case IFT_VIRTUALIPADDRESS:
        return InterfaceType::Virtual;
      default:
        return InterfaceType::Other;
      }
    }

    if (flags & IFF_LOOPBACK)
      return InterfaceType::Loopback;
    if (flags & IFF_POINTOPOINT)
      return InterfaceType::PointToPoint;

    return InterfaceType::Unknown;
  }

} // anonymous namespace

void SystemConfigurationManager::prepare_ifreq(struct ifreq &ifr,
                                               const std::string &name) const {
  std::memset(&ifr, 0, sizeof(ifr));
  std::strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ - 1);
}

void SystemConfigurationManager::cloneInterface(const std::string &name,
                                                unsigned long cmd) const {
  Socket sock(AF_INET, SOCK_DGRAM);
  struct ifreq ifr;
  prepare_ifreq(ifr, name);
  if (ioctl(sock, cmd, &ifr) < 0) {
    throw std::runtime_error("Failed to create interface '" + name +
                             "': " + std::string(strerror(errno)));
  }
}

std::optional<int> SystemConfigurationManager::query_ifreq_int(
    const std::string &ifname, unsigned long req, IfreqIntField which) const {
  try {
    Socket s(AF_INET, SOCK_DGRAM);
    struct ifreq ifr;
    prepare_ifreq(ifr, ifname);
    if (ioctl(s, req, &ifr) == 0) {
      int v = 0;
      switch (which) {
      case IfreqIntField::Metric:
        v = ifr.ifr_metric;
        break;
      case IfreqIntField::Fib:
        v = ifr.ifr_fib;
        break;
      case IfreqIntField::Mtu:
        v = ifr.ifr_mtu;
        break;
      }
      return v;
    }
  } catch (...) {
  }
  return std::nullopt;
}

std::optional<std::pair<std::string, int>>
SystemConfigurationManager::query_ifreq_sockaddr(const std::string &ifname,
                                                 unsigned long req) const {
  try {
    Socket s(AF_INET, SOCK_DGRAM);
    struct ifreq ifr;
    prepare_ifreq(ifr, ifname);
    if (ioctl(s, req, &ifr) == 0) {
      if (ifr.ifr_addr.sa_family == AF_INET) {
        struct sockaddr_in *sin =
            reinterpret_cast<struct sockaddr_in *>(&ifr.ifr_addr);
        char buf[INET_ADDRSTRLEN] = {0};
        if (inet_ntop(AF_INET, &sin->sin_addr, buf, sizeof(buf)))
          return std::make_pair(std::string(buf), AF_INET);
      } else if (ifr.ifr_addr.sa_family == AF_INET6) {
        struct sockaddr_in6 *sin6 =
            reinterpret_cast<struct sockaddr_in6 *>(&ifr.ifr_addr);
        char buf[INET6_ADDRSTRLEN] = {0};
        if (inet_ntop(AF_INET6, &sin6->sin6_addr, buf, sizeof(buf)))
          return std::make_pair(std::string(buf), AF_INET6);
      }
    }
  } catch (...) {
  }
  return std::nullopt;
}

std::vector<std::string> SystemConfigurationManager::query_interface_groups(
    const std::string &ifname) const {
  std::vector<std::string> out;
  try {
    Socket s(AF_LOCAL, SOCK_DGRAM);
    struct ifgroupreq ifgr;
    std::memset(&ifgr, 0, sizeof(ifgr));
    std::strncpy(ifgr.ifgr_name, ifname.c_str(), IFNAMSIZ - 1);
    if (ioctl(s, SIOCGIFGROUP, &ifgr) == 0) {
      size_t len = ifgr.ifgr_len;
      if (len > 0) {
        size_t count = len / sizeof(struct ifg_req);
        std::vector<struct ifg_req> groups(count);
        ifgr.ifgr_groups = groups.data();
        if (ioctl(s, SIOCGIFGROUP, &ifgr) == 0) {
          for (size_t i = 0; i < count; ++i) {
            std::string gname(groups[i].ifgrq_group);
            if (!gname.empty())
              out.emplace_back(gname);
          }
        }
      }
    }
  } catch (...) {
  }
  return out;
}

namespace {

  // Build an IPNetwork from ifaddrs address/netmask (returns nullptr if not
  // IPv4/6)
  std::unique_ptr<IPNetwork> ipNetworkFromIfa(const struct ifaddrs *ifa) {
    if (!ifa || !ifa->ifa_addr)
      return nullptr;
    char buf[INET6_ADDRSTRLEN] = {0};
    if (ifa->ifa_addr->sa_family == AF_INET) {
      auto *a = reinterpret_cast<struct sockaddr_in *>(ifa->ifa_addr);
      if (inet_ntop(AF_INET, &a->sin_addr, buf, sizeof(buf))) {
        uint8_t masklen = 32;
        if (ifa->ifa_netmask)
          masklen = IPNetwork::masklenFromSockaddr(ifa->ifa_netmask);
        return IPNetwork::fromString(std::string(buf) + "/" +
                                     std::to_string(masklen));
      }
    } else if (ifa->ifa_addr->sa_family == AF_INET6) {
      auto *a6 = reinterpret_cast<struct sockaddr_in6 *>(ifa->ifa_addr);
      if (inet_ntop(AF_INET6, &a6->sin6_addr, buf, sizeof(buf))) {
        uint8_t masklen = 128;
        if (ifa->ifa_netmask)
          masklen = IPNetwork::masklenFromSockaddr(ifa->ifa_netmask);
        return IPNetwork::fromString(std::string(buf) + "/" +
                                     std::to_string(masklen));
      }
    }
    return nullptr;
  }

  // Build a sockaddr_in from an IPv4Address host-order value.
  struct sockaddr_in makeSockaddrIn(uint32_t hostOrder) {
    struct sockaddr_in sa{};
    sa.sin_len = sizeof(sa);
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(hostOrder);
    return sa;
  }

  // Fill a sockaddr_in6 from an IPv6Address 128-bit value.
  struct sockaddr_in6 makeSockaddrIn6(unsigned __int128 v) {
    struct sockaddr_in6 sa6{};
    sa6.sin6_len = sizeof(sa6);
    sa6.sin6_family = AF_INET6;
    for (int i = 15; i >= 0; --i) {
      sa6.sin6_addr.s6_addr[i] = static_cast<uint8_t>(v & 0xFF);
      v >>= 8;
    }
    return sa6;
  }

  // Build an IPv6 prefix mask sockaddr_in6 from a prefix length.
  struct sockaddr_in6 makePrefixMask6(int prefixlen) {
    struct sockaddr_in6 mask6{};
    mask6.sin6_len = sizeof(mask6);
    mask6.sin6_family = AF_INET6;
    for (int i = 0; i < 16; ++i) {
      if (prefixlen >= 8) {
        mask6.sin6_addr.s6_addr[i] = 0xff;
        prefixlen -= 8;
      } else if (prefixlen > 0) {
        mask6.sin6_addr.s6_addr[i] =
            static_cast<uint8_t>((0xff << (8 - prefixlen)) & 0xff);
        prefixlen = 0;
      } else {
        mask6.sin6_addr.s6_addr[i] = 0;
      }
    }
    return mask6;
  }

  // Configure a single IPv6 address on an interface via SIOCAIFADDR_IN6.
  // `flags` controls DAD behaviour (0 = normal DAD, IN6_IFF_NODAD = skip).
  void addIPv6Addr(int sock, const std::string &ifname, const IPv6Network &net,
                   int flags) {
    auto addr = net.address();
    auto *v6 = dynamic_cast<IPv6Address *>(addr.get());
    if (!v6)
      return;

    struct in6_aliasreq iar6{};
    std::strncpy(iar6.ifra_name, ifname.c_str(), IFNAMSIZ - 1);

    auto sa6 = makeSockaddrIn6(v6->value());
    std::memcpy(&iar6.ifra_addr, &sa6, sizeof(sa6));

    auto mask6 = makePrefixMask6(net.mask());
    std::memcpy(&iar6.ifra_prefixmask, &mask6, sizeof(mask6));

    iar6.ifra_flags = flags;
    iar6.ifra_lifetime.ia6t_vltime = 0xffffffff; // ND6_INFINITE_LIFETIME
    iar6.ifra_lifetime.ia6t_pltime = 0xffffffff;

    if (ioctl(sock, SIOCAIFADDR_IN6, &iar6) < 0) {
      std::cerr << "Warning: SIOCAIFADDR_IN6 failed for " << ifname << ": "
                << strerror(errno) << "\n";
    }
  }

} // anonymous namespace

void SystemConfigurationManager::populateInterfaceMetadata(
    InterfaceConfig &ic) const {
  if (auto m = query_ifreq_int(ic.name, SIOCGIFMETRIC, IfreqIntField::Metric))
    ic.metric = *m;

  if (auto f = query_ifreq_int(ic.name, SIOCGIFFIB, IfreqIntField::Fib)) {
    if (!ic.vrf)
      ic.vrf = std::make_unique<VRFConfig>(*f);
    else
      ic.vrf->table = *f;
  }

  if (auto mtu = query_ifreq_int(ic.name, SIOCGIFMTU, IfreqIntField::Mtu))
    ic.mtu = *mtu;

  if (unsigned int idx = if_nametoindex(ic.name.c_str()); idx != 0)
    ic.index = static_cast<int>(idx);

  ic.groups = query_interface_groups(ic.name);

  // --- Description (SIOCGIFDESCR) ---
  try {
    Socket s(AF_INET, SOCK_DGRAM);
    struct ifreq ifr;
    prepare_ifreq(ifr, ic.name);
    char descbuf[256] = {0};
    ifr.ifr_buffer.buffer = descbuf;
    ifr.ifr_buffer.length = sizeof(descbuf);
    if (ioctl(s, SIOCGIFDESCR, &ifr) == 0 && descbuf[0] != '\0')
      ic.description = std::string(descbuf);
  } catch (...) {
  }

  // --- Hardware / MAC address (AF_LINK from getifaddrs) ---
  {
    struct ifaddrs *ifs = nullptr;
    if (getifaddrs(&ifs) == 0) {
      for (struct ifaddrs *ifa = ifs; ifa; ifa = ifa->ifa_next) {
        if (!ifa->ifa_name || ic.name != ifa->ifa_name)
          continue;
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_LINK) {
          auto *sdl = reinterpret_cast<struct sockaddr_dl *>(ifa->ifa_addr);
          if (sdl->sdl_alen == 6) {
            auto *mac = reinterpret_cast<unsigned char *>(LLADDR(sdl));
            char macbuf[32];
            std::snprintf(macbuf, sizeof(macbuf),
                          "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1],
                          mac[2], mac[3], mac[4], mac[5]);
            // Skip all-zero addresses (e.g. loopback)
            if (std::string(macbuf) != "00:00:00:00:00:00")
              ic.hwaddr = std::string(macbuf);
          }
          // Also harvest if_data fields from AF_LINK entry
          if (ifa->ifa_data) {
            auto *ifd = reinterpret_cast<struct if_data *>(ifa->ifa_data);
            if (ifd->ifi_baudrate > 0)
              ic.baudrate = ifd->ifi_baudrate;
            ic.link_state = ifd->ifi_link_state;
          }
          break;
        }
      }
      freeifaddrs(ifs);
    }
  }

  // --- Capabilities (SIOCGIFCAP) ---
  try {
    Socket s(AF_INET, SOCK_DGRAM);
    struct ifreq ifr;
    prepare_ifreq(ifr, ic.name);
    if (ioctl(s, SIOCGIFCAP, &ifr) == 0) {
      ic.req_capabilities = static_cast<uint32_t>(ifr.ifr_reqcap);
      ic.capabilities = static_cast<uint32_t>(ifr.ifr_curcap);
    }
  } catch (...) {
  }

  // --- Media (SIOCGIFMEDIA) ---
  try {
    Socket s(AF_INET, SOCK_DGRAM);
    struct ifmediareq ifmr{};
    std::strncpy(ifmr.ifm_name, ic.name.c_str(), IFNAMSIZ - 1);
    if (ioctl(s, SIOCGIFMEDIA, &ifmr) == 0) {
      ic.media_status = ifmr.ifm_status;
      // Store raw current/active words; higher-level formatting is done
      // by the formatter layer which can map IFM_* to human-readable text.
      ic.media = std::to_string(ifmr.ifm_current);
      ic.media_active = std::to_string(ifmr.ifm_active);
    }
  } catch (...) {
  }

  // --- Driver status string (SIOCGIFSTATUS) ---
  try {
    Socket s(AF_INET, SOCK_DGRAM);
    struct ifstat ifs{};
    std::strncpy(ifs.ifs_name, ic.name.c_str(), IFNAMSIZ - 1);
    if (ioctl(s, SIOCGIFSTATUS, &ifs) == 0 && ifs.ascii[0] != '\0')
      ic.status_str = std::string(ifs.ascii);
  } catch (...) {
  }

  // --- Physical wire type (SIOCGIFPHYS) ---
  try {
    Socket s(AF_INET, SOCK_DGRAM);
    struct ifreq ifr;
    prepare_ifreq(ifr, ic.name);
    if (ioctl(s, SIOCGIFPHYS, &ifr) == 0)
      ic.phys = ifr.ifr_phys;
  } catch (...) {
  }
}

void SystemConfigurationManager::SaveInterface(
    const InterfaceConfig &ic) const {
  if (ic.name.empty())
    throw std::runtime_error("Interface has no name");

  if (!InterfaceConfig::exists(*this, ic.name))
    CreateInterface(ic.name);

  Socket sock(AF_INET, SOCK_DGRAM);

  // --- Address configuration ---
  if (ic.address || !ic.aliases.empty()) {
    // Primary IPv4
    if (ic.address && ic.address->family() == AddressFamily::IPv4) {
      auto *v4 = dynamic_cast<IPv4Network *>(ic.address.get());
      if (v4) {
        auto addr = v4->address();
        auto *v4a = dynamic_cast<IPv4Address *>(addr.get());
        if (v4a) {
          struct ifreq aifr;
          prepare_ifreq(aifr, ic.name);
          auto sa = makeSockaddrIn(v4a->value());
          std::memcpy(&aifr.ifr_addr, &sa, sizeof(sa));
          if (ioctl(sock, SIOCSIFADDR, &aifr) < 0)
            std::cerr << "Warning: SIOCSIFADDR failed for " << ic.name << ": "
                      << strerror(errno) << "\n";
        }
      }
    }

    // IPv4 aliases
    for (const auto &aptr : ic.aliases) {
      if (!aptr || aptr->family() != AddressFamily::IPv4)
        continue;
      auto *a4net = dynamic_cast<IPv4Network *>(aptr.get());
      if (!a4net)
        continue;
      auto addr = a4net->address();
      auto *v4a = dynamic_cast<IPv4Address *>(addr.get());
      if (!v4a)
        continue;

      struct ifaliasreq iar{};
      std::strncpy(iar.ifra_name, ic.name.c_str(), IFNAMSIZ - 1);

      auto sa = makeSockaddrIn(v4a->value());
      std::memcpy(&iar.ifra_addr, &sa, sizeof(sa));

      uint32_t maskval =
          (a4net->mask() == 0) ? 0u : (~0u << (32 - a4net->mask()));
      auto mask = makeSockaddrIn(maskval);
      std::memcpy(&iar.ifra_mask, &mask, sizeof(mask));

      uint32_t bcast = (v4a->value() & maskval) | ~maskval;
      auto broad = makeSockaddrIn(bcast);
      std::memcpy(&iar.ifra_broadaddr, &broad, sizeof(broad));

      if (ioctl(sock, SIOCAIFADDR, &iar) < 0) {
        // Fallback: set address only
        struct ifreq rifr;
        prepare_ifreq(rifr, ic.name);
        std::memcpy(&rifr.ifr_addr, &sa, sizeof(sa));
        if (ioctl(sock, SIOCSIFADDR, &rifr) < 0) {
          std::cerr << "Warning: SIOCSIFADDR failed when adding alias to "
                    << ic.name << ": " << strerror(errno) << "\n";
          continue;
        }
      }
    }

    // Primary IPv6
    if (ic.address && ic.address->family() == AddressFamily::IPv6) {
      auto *v6 = dynamic_cast<IPv6Network *>(ic.address.get());
      if (v6) {
        try {
          Socket sock6(AF_INET6, SOCK_DGRAM);
          addIPv6Addr(sock6, ic.name, *v6, 0 /* let kernel handle DAD */);
        } catch (const std::exception &e) {
          std::cerr << "Warning: " << e.what() << "\n";
        }
      }
    }

    // IPv6 aliases
    for (const auto &aptr : ic.aliases) {
      if (!aptr || aptr->family() != AddressFamily::IPv6)
        continue;
      auto *a6net = dynamic_cast<IPv6Network *>(aptr.get());
      if (a6net)
        addIPv6Addr(sock, ic.name, *a6net, 0x20 /* IN6_IFF_NODAD */);
    }
  }

  // --- MTU ---
  if (ic.mtu) {
    struct ifreq ifr;
    prepare_ifreq(ifr, ic.name);
    ifr.ifr_mtu = *ic.mtu;
    if (ioctl(sock, SIOCSIFMTU, &ifr) < 0)
      throw std::runtime_error("Failed to set MTU on " + ic.name + ": " +
                               std::string(strerror(errno)));
  }

  // --- Bring interface up ---
  {
    struct ifreq ifr;
    prepare_ifreq(ifr, ic.name);
    if (ioctl(sock, SIOCGIFFLAGS, &ifr) >= 0) {
      ifr.ifr_flags |= IFF_UP;
      if (ioctl(sock, SIOCSIFFLAGS, &ifr) < 0)
        throw std::runtime_error("Failed to bring interface up: " +
                                 std::string(strerror(errno)));
    }
  }

  // --- VRF / FIB ---
  if (ic.vrf && ic.vrf->table) {
    struct ifreq ifr;
    prepare_ifreq(ifr, ic.name);
    ifr.ifr_fib = ic.vrf->table;
    if (ioctl(sock, SIOCSIFFIB, &ifr) < 0)
      throw std::runtime_error("Failed to set interface FIB: " +
                               std::string(strerror(errno)));
  }

  // --- Interface groups (add only new) ---
  auto existing = query_interface_groups(ic.name);
  for (const auto &group : ic.groups) {
    if (std::ranges::contains(existing, group))
      continue;

    struct ifgroupreq ifgr{};
    std::strncpy(ifgr.ifgr_name, ic.name.c_str(), IFNAMSIZ - 1);
    std::strncpy(ifgr.ifgr_group, group.c_str(), IFNAMSIZ - 1);
    if (ioctl(sock, SIOCAIFGROUP, &ifgr) < 0)
      throw std::runtime_error("Failed to add interface group '" + group +
                               "': " + std::string(strerror(errno)));
  }

  // --- Description (SIOCSIFDESCR) ---
  if (ic.description) {
    struct ifreq ifr;
    prepare_ifreq(ifr, ic.name);
    ifr.ifr_buffer.buffer = const_cast<char *>(ic.description->c_str());
    ifr.ifr_buffer.length = ic.description->size() + 1;
    if (ioctl(sock, SIOCSIFDESCR, &ifr) < 0)
      std::cerr << "Warning: SIOCSIFDESCR failed for " << ic.name << ": "
                << strerror(errno) << "\n";
  }

  // --- Capabilities (SIOCSIFCAP) ---
  if (ic.req_capabilities) {
    struct ifreq ifr;
    prepare_ifreq(ifr, ic.name);
    ifr.ifr_reqcap = static_cast<int>(*ic.req_capabilities);
    if (ioctl(sock, SIOCSIFCAP, &ifr) < 0)
      std::cerr << "Warning: SIOCSIFCAP failed for " << ic.name << ": "
                << strerror(errno) << "\n";
  }

  // --- Metric (SIOCSIFMETRIC) ---
  if (ic.metric) {
    struct ifreq ifr;
    prepare_ifreq(ifr, ic.name);
    ifr.ifr_metric = *ic.metric;
    if (ioctl(sock, SIOCSIFMETRIC, &ifr) < 0)
      std::cerr << "Warning: SIOCSIFMETRIC failed for " << ic.name << ": "
                << strerror(errno) << "\n";
  }
}

bool SystemConfigurationManager::InterfaceExists(std::string_view name) const {
  return if_nametoindex(std::string(name).c_str()) != 0;
}

std::vector<std::string>
SystemConfigurationManager::GetInterfaceAddresses(const std::string &ifname,
                                                  int family) const {
  std::vector<std::string> out;
  struct ifaddrs *ifap = nullptr;
  if (getifaddrs(&ifap) == 0) {
    for (struct ifaddrs *ifa = ifap; ifa; ifa = ifa->ifa_next) {
      if (!ifa->ifa_name)
        continue;
      if (ifname != ifa->ifa_name)
        continue;
      if (!ifa->ifa_addr)
        continue;
      if (family == AF_INET && ifa->ifa_addr->sa_family == AF_INET) {
        char buf[INET_ADDRSTRLEN] = {0};
        struct sockaddr_in *sin =
            reinterpret_cast<struct sockaddr_in *>(ifa->ifa_addr);
        if (inet_ntop(AF_INET, &sin->sin_addr, buf, sizeof(buf))) {
          out.emplace_back(std::string(buf) + "/32");
        }
      } else if (family == AF_INET6 && ifa->ifa_addr->sa_family == AF_INET6) {
        char buf[INET6_ADDRSTRLEN] = {0};
        struct sockaddr_in6 *sin6 =
            reinterpret_cast<struct sockaddr_in6 *>(ifa->ifa_addr);
        if (inet_ntop(AF_INET6, &sin6->sin6_addr, buf, sizeof(buf))) {
          out.emplace_back(std::string(buf) + "/128");
        }
      }
    }
    freeifaddrs(ifap);
  }
  return out;
}

void SystemConfigurationManager::DestroyInterface(
    const std::string &name) const {
  if (name.empty())
    throw std::runtime_error(
        "InterfaceConfig::destroy(): empty interface name");

  Socket sock(AF_INET, SOCK_DGRAM);
  struct ifreq ifr;
  prepare_ifreq(ifr, name);
  if (ioctl(sock, SIOCIFDESTROY, &ifr) < 0) {
    throw std::runtime_error("Failed to destroy interface '" + name +
                             "': " + std::string(strerror(errno)));
  }
}

void SystemConfigurationManager::RemoveInterfaceAddress(
    const std::string &ifname, const std::string &addr) const {
  auto net = IPNetwork::fromString(addr);
  if (!net)
    throw std::runtime_error("Invalid address: " + addr);

  if (net->family() == AddressFamily::IPv4) {
    auto *v4 = dynamic_cast<IPv4Network *>(net.get());
    if (!v4)
      throw std::runtime_error("Invalid IPv4 address object");

    auto ipaddr = v4->address();
    auto *v4a = dynamic_cast<IPv4Address *>(ipaddr.get());
    if (!v4a)
      throw std::runtime_error("Invalid IPv4 address object");

    Socket sock(AF_INET, SOCK_DGRAM);
    struct ifreq ifr;
    prepare_ifreq(ifr, ifname);
    auto sa = makeSockaddrIn(v4a->value());
    std::memcpy(&ifr.ifr_addr, &sa, sizeof(sa));
    if (ioctl(sock, SIOCDIFADDR, &ifr) < 0)
      throw std::runtime_error(std::string("failed to remove address: ") +
                               strerror(errno));
    return;
  }

  throw std::runtime_error("IPv6 address removal not implemented: " + addr);
}

void SystemConfigurationManager::RemoveInterfaceGroup(
    const std::string &ifname, const std::string &group) const {
  Socket sock(AF_LOCAL, SOCK_DGRAM);

  struct ifgroupreq ifgr{};
  std::strncpy(ifgr.ifgr_name, ifname.c_str(), IFNAMSIZ - 1);
  std::strncpy(ifgr.ifgr_group, group.c_str(), IFNAMSIZ - 1);

  if (ioctl(sock, SIOCDIFGROUP, &ifgr) < 0)
    throw std::runtime_error("Failed to remove group '" + group +
                             "': " + std::string(strerror(errno)));
}

void SystemConfigurationManager::CreateInterface(
    const std::string &name) const {
  cloneInterface(name, SIOCIFCREATE);
}

std::vector<InterfaceConfig> SystemConfigurationManager::GetInterfaces(
    const std::optional<VRFConfig> &vrf) const {
  std::vector<InterfaceConfig> out;
  struct ifaddrs *ifs = nullptr;
  if (getifaddrs(&ifs) != 0)
    return out;

  std::unordered_map<std::string, std::shared_ptr<InterfaceConfig>> map;
  for (struct ifaddrs *ifa = ifs; ifa; ifa = ifa->ifa_next) {
    if (!ifa->ifa_name)
      continue;
    std::string name = ifa->ifa_name;
    auto it = map.find(name);
    if (it == map.end()) {
      // Build primary InterfaceConfig from ifa fields (system-level parsing)
      InterfaceType t = ifAddrToInterfaceType(ifa);
      std::unique_ptr<IPNetwork> addr = ipNetworkFromIfa(ifa);
      std::vector<std::unique_ptr<IPNetwork>> aliases;
      std::optional<uint32_t> flags = std::nullopt;
      if (ifa->ifa_flags)
        flags = ifa->ifa_flags;
      auto ic = std::shared_ptr<InterfaceConfig>(
          new InterfaceConfig(name, t, std::move(addr), std::move(aliases),
                              nullptr, flags, {}, std::nullopt));
      if (ic->type == InterfaceType::Wireless) {
        auto w = std::shared_ptr<WlanInterfaceConfig>(new WlanInterfaceConfig(*ic));
        map.emplace(name, std::move(w));
      } else {
        map.emplace(name, std::move(ic));
      }
    } else {
      auto existing = it->second;
      if (ifa->ifa_addr) {
        if (ifa->ifa_addr->sa_family == AF_INET ||
            ifa->ifa_addr->sa_family == AF_INET6) {
          auto tmpaddr = ipNetworkFromIfa(ifa);
          if (tmpaddr) {
            if (!existing->address) {
              existing->address = std::move(tmpaddr);
            } else {
              existing->aliases.emplace_back(std::move(tmpaddr));
            }
          }
        }
      }
    }
  }

  for (auto &kv : map) {
    populateInterfaceMetadata(*kv.second);
    for (const auto &g : kv.second->groups) {
      if (g == "epair") {
        kv.second->type = InterfaceType::Virtual;
        break;
      }
    }
    if (interfaceIsLagg(kv.second->name)) {
      kv.second->type = InterfaceType::Lagg;
    } else if (interfaceIsBridge(kv.second->name)) {
      kv.second->type = InterfaceType::Bridge;
    } else if (kv.second->name.rfind("gre", 0) == 0) {
      kv.second->type = InterfaceType::GRE;
    } else if (kv.second->name.rfind("vxlan", 0) == 0) {
      kv.second->type = InterfaceType::VXLAN;
    } else if (kv.second->name.rfind("ipsec", 0) == 0) {
      kv.second->type = InterfaceType::IPsec;
    } else if (kv.second->name.rfind("wlan", 0) == 0) {
      kv.second->type = InterfaceType::Wireless;
    }
    if (matches_vrf(*kv.second, vrf))
      out.emplace_back(std::move(*kv.second));
  }

  freeifaddrs(ifs);
  return out;
}

std::vector<InterfaceConfig> SystemConfigurationManager::GetInterfacesByGroup(
    const std::optional<VRFConfig> &vrf, std::string_view group) const {
  auto bases = GetInterfaces(vrf);
  std::vector<InterfaceConfig> out;
  for (auto &ic : bases) {
    if (std::ranges::contains(ic.groups, group))
      out.push_back(std::move(ic));
  }
  return out;
}
