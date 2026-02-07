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

#include "ConfigurationManager.hpp"
#include "IPNetwork.hpp"
#include "InterfaceConfig.hpp"
#include "InterfaceToken.hpp"
#include "Parser.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <ifaddrs.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/sockio.h>
#include <unistd.h>

void netcli::Parser::executeDeleteInterface(const InterfaceToken &tok,
                                            ConfigurationManager *mgr) const {
  (void)mgr;

  const std::string name = tok.name();
  if (name.empty()) {
    std::cerr << "delete interface: missing interface name\n";
    return;
  }

  if (!InterfaceConfig::exists(name)) {
    std::cerr << "delete interface: interface '" << name << "' not found\n";
    return;
  }

  InterfaceConfig ic;
  ic.name = name;
  try {
    // If token requests address-level deletion (inet/inet6), handle
    // targeted removal rather than destroying the entire interface.
    if (tok.address || tok.address_family) {
      // Build list of addresses to remove. If a specific address string was
      // provided, remove just that; otherwise remove all addresses matching
      // the requested family (currently handles AF_INET and AF_INET6
      // best-effort).
      std::vector<std::string> to_remove;
      if (tok.address) {
        to_remove.push_back(*tok.address);
      } else if (tok.address_family && *tok.address_family == AF_INET) {
        // enumerate IPv4 addresses on the interface
        struct ifaddrs *ifap = nullptr;
        if (getifaddrs(&ifap) == 0) {
          for (struct ifaddrs *ifa = ifap; ifa; ifa = ifa->ifa_next) {
            if (!ifa->ifa_name)
              continue;
            if (name != ifa->ifa_name)
              continue;
            if (!ifa->ifa_addr)
              continue;
            if (ifa->ifa_addr->sa_family == AF_INET) {
              char buf[INET_ADDRSTRLEN] = {0};
              struct sockaddr_in *sin =
                  reinterpret_cast<struct sockaddr_in *>(ifa->ifa_addr);
              if (inet_ntop(AF_INET, &sin->sin_addr, buf, sizeof(buf))) {
                std::string addr(buf);
                // Add a /32 suffix to indicate single address removal
                to_remove.emplace_back(addr + "/32");
              }
            }
          }
          freeifaddrs(ifap);
        }
      } else if (tok.address_family && *tok.address_family == AF_INET6) {
        // enumerate IPv6 addresses
        struct ifaddrs *ifap = nullptr;
        if (getifaddrs(&ifap) == 0) {
          for (struct ifaddrs *ifa = ifap; ifa; ifa = ifa->ifa_next) {
            if (!ifa->ifa_name)
              continue;
            if (name != ifa->ifa_name)
              continue;
            if (!ifa->ifa_addr)
              continue;
            if (ifa->ifa_addr->sa_family == AF_INET6) {
              char buf[INET6_ADDRSTRLEN] = {0};
              struct sockaddr_in6 *sin6 =
                  reinterpret_cast<struct sockaddr_in6 *>(ifa->ifa_addr);
              if (inet_ntop(AF_INET6, &sin6->sin6_addr, buf, sizeof(buf))) {
                std::string addr(buf);
                to_remove.emplace_back(addr + "/128");
              }
            }
          }
          freeifaddrs(ifap);
        }
      }

      int sock = socket(AF_INET, SOCK_DGRAM, 0);
      if (sock < 0) {
        std::cerr << "delete interface: failed to create socket: "
                  << strerror(errno) << "\n";
      } else {
        for (const auto &a : to_remove) {
          auto net = IPNetwork::fromString(a);
          if (!net)
            continue;
          if (net->family() == AddressFamily::IPv4) {
            auto v4 = dynamic_cast<IPv4Network *>(net.get());
            if (!v4)
              continue;
            struct ifreq ifr;
            std::memset(&ifr, 0, sizeof(ifr));
            std::strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ - 1);
            struct sockaddr_in *sin =
                reinterpret_cast<struct sockaddr_in *>(&ifr.ifr_addr);
            sin->sin_family = AF_INET;
            struct in_addr a4;
            auto ipaddr = v4->address();
            auto ipv4addr = dynamic_cast<IPv4Address *>(ipaddr.get());
            if (!ipv4addr) {
              std::cerr << "delete interface: invalid IPv4 address object\n";
              continue;
            }
            a4.s_addr = htonl(ipv4addr->value());
            sin->sin_addr = a4;
            if (ioctl(sock, SIOCDIFADDR, &ifr) < 0) {
              std::cerr << "delete interface: failed to remove address '" << a
                        << "': " << strerror(errno) << "\n";
            } else {
              std::cout << "delete interface: removed address '" << a
                        << "' from '" << name << "'\n";
            }
          } else if (net->family() == AddressFamily::IPv6) {
            // Best-effort: attempt SIOCDIFADDR with sockaddr_in6
            struct ifreq ifr;
            std::memset(&ifr, 0, sizeof(ifr));
            std::strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ - 1);
            // Copy sockaddr_in6 into ifr.ifr_addr (may not be portable);
            // fallback: report cannot remove
            std::cerr << "delete interface: IPv6 address removal not fully "
                      << "implemented for '" << a << "'\n";
          }
        }
        close(sock);
      }
      return;
    }

    // Default: destroy the interface
    ic.destroy();
    std::cout << "delete interface: removed '" << name << "'\n";
  } catch (const std::exception &e) {
    std::cerr << "delete interface: failed to remove '" << name
              << "': " << e.what() << "\n";
  }
}
