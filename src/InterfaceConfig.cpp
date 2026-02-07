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

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <ifaddrs.h>
#include <iostream>
#include <net/if.h>
#include <netinet/in.h>
#include <optional>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <unistd.h>

#include "IPAddress.hpp"
#include "IPNetwork.hpp"

#include "VRFConfig.hpp"

// (no helpers) prepare `ifreq` inline where needed

InterfaceConfig::InterfaceConfig(const struct ifaddrs *ifa) {
  if (!ifa || !ifa->ifa_name)
    return;

  name = ifa->ifa_name;
  type = ifAddrToInterfaceType(ifa);

  // Prefer explicit kernel-provided link-layer type. Do not infer type
  // from the interface name (avoid brittle name-prefix heuristics).

  // Store flags if present
  if (ifa->ifa_flags) {
    flags = ifa->ifa_flags;
  }

  // Attempt to populate numeric interface index using if_nametoindex()
  unsigned int ifidx = if_nametoindex(name.c_str());
  if (ifidx != 0)
    index = static_cast<int>(ifidx);

  // Get MTU
  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock >= 0) {
    struct ifreq ifr;
    std::memset(&ifr, 0, sizeof(ifr));
    std::strncpy(ifr.ifr_name, ifa->ifa_name, IFNAMSIZ - 1);
    if (ioctl(sock, SIOCGIFMTU, &ifr) == 0) {
      mtu = ifr.ifr_mtu;
    }
    close(sock);
  }

  if (ifa->ifa_addr) {
    char buf[INET6_ADDRSTRLEN] = {0};
    if (ifa->ifa_addr->sa_family == AF_INET) {
      struct sockaddr_in *a =
          reinterpret_cast<struct sockaddr_in *>(ifa->ifa_addr);
      if (inet_ntop(AF_INET, &a->sin_addr, buf, sizeof(buf))) {
        uint8_t masklen = 32;
        if (ifa->ifa_netmask)
          masklen = IPNetwork::masklenFromSockaddr(ifa->ifa_netmask);
        address = IPNetwork::fromString(std::string(buf) + "/" +
                                        std::to_string(masklen));
      }
    } else if (ifa->ifa_addr->sa_family == AF_INET6) {
      struct sockaddr_in6 *a6 =
          reinterpret_cast<struct sockaddr_in6 *>(ifa->ifa_addr);
      if (inet_ntop(AF_INET6, &a6->sin6_addr, buf, sizeof(buf))) {
        uint8_t masklen = 128;
        if (ifa->ifa_netmask)
          masklen = IPNetwork::masklenFromSockaddr(ifa->ifa_netmask);
        address = IPNetwork::fromString(std::string(buf) + "/" +
                                        std::to_string(masklen));
      }
    }
  }
}

InterfaceConfig::InterfaceConfig(
    std::string name_, InterfaceType type_, std::unique_ptr<IPNetwork> address_,
    std::vector<std::unique_ptr<IPNetwork>> aliases_,
    std::unique_ptr<VRFConfig> vrf_, std::optional<uint32_t> flags_,
    std::vector<std::string> groups_, std::optional<int> mtu_)
    : name(std::move(name_)), type(type_), address(std::move(address_)),
      aliases(std::move(aliases_)), vrf(std::move(vrf_)), flags(flags_),
      groups(std::move(groups_)), mtu(mtu_) {}

void InterfaceConfig::save() const {
  // Do not create interfaces here; subclasses handle creation if necessary.
  if (!InterfaceConfig::exists(name)) {
    throw std::runtime_error("Interface '" + name + "' does not exist");
  }

  // Configure address, MTU, flags, and other settings
  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0) {
    throw std::runtime_error("Failed to create socket: " +
                             std::string(strerror(errno)));
  }

  // Configure addresses (primary + aliases) if provided
  if (address || !aliases.empty()) {
    struct ifreq ifr;
    std::memset(&ifr, 0, sizeof(ifr));
    std::strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ - 1);
    configureAddresses(sock, ifr);
  }

  // Configure MTU if provided
  if (mtu) {
    struct ifreq ifr;
    std::memset(&ifr, 0, sizeof(ifr));
    std::strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ - 1);
    configureMTU(sock, ifr);
  }

  // Bring interface up / configure flags

  struct ifreq ifr;
  std::memset(&ifr, 0, sizeof(ifr));
  std::strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ - 1);
  configureFlags(sock, ifr);

  // Apply VRF / FIB to the interface if requested
  if (vrf && vrf->table) {
    struct ifreq fib_ifr;
    std::memset(&fib_ifr, 0, sizeof(fib_ifr));
    std::strncpy(fib_ifr.ifr_name, name.c_str(), IFNAMSIZ - 1);
    fib_ifr.ifr_fib = *vrf->table;
    if (ioctl(sock, SIOCSIFFIB, &fib_ifr) < 0) {
      int err = errno;
      close(sock);
      throw std::runtime_error("Failed to set interface FIB: " +
                               std::string(strerror(err)));
    }
  }

  close(sock);
}

// Interface existence helper is provided by `ConfigData::exists`

void InterfaceConfig::configureAddresses(int sock, struct ifreq &ifr) const {
  (void)ifr; // caller provided a generic ifr; we build per-ioctl structs here

  // Configure primary IPv4 address if requested
  if (address && address->family() == AddressFamily::IPv4) {
    auto v4 = dynamic_cast<IPv4Network *>(address.get());
    if (v4) {
      auto netAddr = v4->address();
      auto v4addr = dynamic_cast<IPv4Address *>(netAddr.get());
      if (v4addr) {
        struct sockaddr_in sa;
        std::memset(&sa, 0, sizeof(sa));
        sa.sin_len = sizeof(sa);
        sa.sin_family = AF_INET;
        sa.sin_addr.s_addr = htonl(v4addr->value());

        struct ifreq aifr;
        std::memset(&aifr, 0, sizeof(aifr));
        std::strncpy(aifr.ifr_name, name.c_str(), IFNAMSIZ - 1);
        std::memcpy(&aifr.ifr_addr, &sa, sizeof(sa));
        if (ioctl(sock, SIOCSIFADDR, &aifr) < 0) {
          std::cerr << "Warning: SIOCSIFADDR failed for " << name << ": "
                    << strerror(errno) << "\n";
        }

        // (netmask and /31 destination handling intentionally omitted)
      }
    }
  }

  // Configure IPv4 aliases
  for (const auto &aptr : aliases) {
    if (!aptr)
      continue;
    if (aptr->family() != AddressFamily::IPv4)
      continue;
    auto a4net = dynamic_cast<IPv4Network *>(aptr.get());
    if (!a4net)
      continue;

    struct ifaliasreq iar;
    std::memset(&iar, 0, sizeof(iar));
    std::strncpy(iar.ifra_name, name.c_str(), IFNAMSIZ - 1);

    struct sockaddr_in sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sin_len = sizeof(sa);
    sa.sin_family = AF_INET;
    auto netAddr = a4net->address();
    auto v4addr = dynamic_cast<IPv4Address *>(netAddr.get());
    if (!v4addr)
      continue;
    sa.sin_addr.s_addr = htonl(v4addr->value());
    std::memcpy(&iar.ifra_addr, &sa, sizeof(sa));

    uint32_t maskval =
        (a4net->mask() == 0) ? 0u : (~0u << (32 - a4net->mask()));
    struct sockaddr_in mask;
    std::memset(&mask, 0, sizeof(mask));
    mask.sin_len = sizeof(mask);
    mask.sin_family = AF_INET;
    mask.sin_addr.s_addr = htonl(maskval);
    std::memcpy(&iar.ifra_mask, &mask, sizeof(mask));

    uint32_t host = v4addr->value();
    uint32_t netn = host & maskval;
    uint32_t bcast = netn | (~maskval);
    struct sockaddr_in broad;
    std::memset(&broad, 0, sizeof(broad));
    broad.sin_len = sizeof(broad);
    broad.sin_family = AF_INET;
    broad.sin_addr.s_addr = htonl(bcast);
    std::memcpy(&iar.ifra_broadaddr, &broad, sizeof(broad));

    if (ioctl(sock, SIOCAIFADDR, &iar) < 0) {
      // fallback to set addr only; netmask and /31 dst handling intentionally
      // omitted
      struct ifreq rifr;
      std::memset(&rifr, 0, sizeof(rifr));
      std::strncpy(rifr.ifr_name, name.c_str(), IFNAMSIZ - 1);
      std::memcpy(&rifr.ifr_addr, &sa, sizeof(sa));
      if (ioctl(sock, SIOCSIFADDR, &rifr) < 0) {
        std::cerr << "Warning: SIOCSIFADDR failed when adding alias to " << name
                  << ": " << strerror(errno) << "\n";
        continue;
      }
    }
  }
}

void InterfaceConfig::configureFlags(int sock, struct ifreq &ifr) const {
  if (ioctl(sock, SIOCGIFFLAGS, &ifr) >= 0) {
    ifr.ifr_flags |= IFF_UP;
    if (ioctl(sock, SIOCSIFFLAGS, &ifr) < 0) {
      std::cerr << "Warning: Failed to bring interface up: " << strerror(errno)
                << "\n";
    }
  }
}

void InterfaceConfig::configureMTU(int sock, struct ifreq &ifr) const {
  if (!mtu)
    return;
  ifr.ifr_mtu = *mtu;
  if (ioctl(sock, SIOCSIFMTU, &ifr) < 0) {
    std::cerr << "Warning: Failed to set MTU on " << name << ": "
              << strerror(errno) << "\n";
  }
}

void InterfaceConfig::destroy() const {
  if (name.empty())
    throw std::runtime_error(
        "InterfaceConfig::destroy(): empty interface name");

  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0) {
    throw std::runtime_error("Failed to create socket: " +
                             std::string(strerror(errno)));
  }

  struct ifreq ifr;
  std::memset(&ifr, 0, sizeof(ifr));
  std::strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ - 1);

  if (ioctl(sock, SIOCIFDESTROY, &ifr) < 0) {
    int err = errno;
    close(sock);
    throw std::runtime_error("Failed to destroy interface '" + name +
                             "': " + std::string(strerror(err)));
  }

  close(sock);
}

// Copy constructor and assignment defined out-of-line to avoid instantiating
// unique_ptr destructors against incomplete types in headers.
InterfaceConfig::InterfaceConfig(const InterfaceConfig &o) {
  name = o.name;
  type = o.type;
  if (o.address)
    address = o.address->clone();
  else
    address.reset();
  aliases.clear();
  for (const auto &a : o.aliases) {
    if (a)
      aliases.emplace_back(a->clone());
    else
      aliases.emplace_back(nullptr);
  }
  if (o.vrf)
    vrf = std::make_unique<VRFConfig>(*o.vrf);
  else
    vrf.reset();
  flags = o.flags;
  index = o.index;
  groups = o.groups;
  mtu = o.mtu;
  (void)0; // wireless attributes moved to WlanConfig
}

// Static helper: check whether a named interface exists on the system.
bool InterfaceConfig::exists(std::string_view name) {
  bool found = false;
  struct ifaddrs *ifs = nullptr;
  if (getifaddrs(&ifs) == 0) {
    for (struct ifaddrs *ifa = ifs; ifa; ifa = ifa->ifa_next) {
      if (ifa->ifa_name && name == ifa->ifa_name) {
        found = true;
        break;
      }
    }
    freeifaddrs(ifs);
  }
  return found;
}
