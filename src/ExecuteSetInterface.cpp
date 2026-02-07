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

#include "BridgeInterfaceConfig.hpp"
#include "ConfigurationManager.hpp"
#include "IPAddress.hpp"
#include "IPNetwork.hpp"
#include "InterfaceConfig.hpp"
#include "InterfaceToken.hpp"
#include "LaggConfig.hpp"
#include "LoopBackConfig.hpp"
#include "Parser.hpp"
#include "TunnelConfig.hpp"
#include "VLANConfig.hpp"
#include "VirtualInterfaceConfig.hpp"
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/sockio.h>
#include <unistd.h>

void netcli::Parser::executeSetInterface(const InterfaceToken &tok,
                                         ConfigurationManager *mgr) const {
  const std::string name = tok.name();
  if (name.empty()) {
    std::cerr << "set interface: missing interface name\n";
    return;
  }

  auto itype = tok.type();
  try {
    // Load existing configuration if present (so we can update), otherwise
    // prepare a new base for creation. Initialize via copy-construction
    // to avoid relying on assignment operators.
    bool exists = InterfaceConfig::exists(name);
    auto ifopt = (exists && mgr) ? mgr->getInterface(name)
                                 : std::optional<InterfaceConfig>{};
    InterfaceConfig base = ifopt ? *ifopt : InterfaceConfig();
    if (!ifopt)
      base.name = name;

    // If the token requested a specific VRF/FIB table, apply it to the base
    if (tok.vrf) {
      if (!base.vrf)
        base.vrf = std::make_unique<VRFConfig>(*tok.vrf);
      else
        base.vrf->table = *tok.vrf;
    }

    // Decide effective type: token > existing > name prefix
    InterfaceType effectiveType = InterfaceType::Unknown;
    if (itype != InterfaceType::Unknown)
      effectiveType = itype;
    else if (base.type != InterfaceType::Unknown)
      effectiveType = base.type;
    else if (base.name.rfind("lo", 0) == 0)
      effectiveType = InterfaceType::Loopback;

    // Handle concrete types
    // Apply address token (if present): add as primary if none, otherwise
    // append
    if (tok.address) {
      auto net = IPNetwork::fromString(*tok.address);
      if (net) {
        if (!base.address) {
          base.address = std::move(net);
        } else {
          base.aliases.emplace_back(net->clone());
        }
      } else {
        std::cerr << "set interface: invalid address '" << *tok.address
                  << "'\n";
      }
    }

    if (effectiveType == InterfaceType::Bridge) {
      BridgeInterfaceConfig bic(base);
      bic.save();
      std::cout << "set interface: " << (exists ? "updated" : "created")
                << " bridge '" << name << "'\n";
      return;
    }

    if (effectiveType == InterfaceType::Lagg) {
      if (!tok.lagg || tok.lagg->members.empty()) {
        std::cerr << "set interface: LAGG creation typically requires member "
                     "interfaces.\n"
                  << "Usage: set interface name <lagg_name> lagg members "
                     "<if1,if2,...> [protocol <proto>]\n";
        return;
      }
      LaggConfig lac(base, tok.lagg->protocol, tok.lagg->members,
                     tok.lagg->hash_policy, tok.lagg->lacp_rate,
                     tok.lagg->min_links);
      lac.save();
      std::cout << "set interface: " << (exists ? "updated" : "created")
                << " lagg '" << name << "'\n";
      return;
    }

    if (effectiveType == InterfaceType::VLAN) {
      if (!tok.vlan || tok.vlan->id == 0 || !tok.vlan->parent) {
        std::cerr << "set interface: VLAN creation requires VLAN id and parent "
                     "interface.\n"
                  << "Usage: set interface name <vlan_name> vlan id <vlan_id> "
                     "parent <parent_iface>\n";
        return;
      }
      VLANConfig vc(base, tok.vlan->id, tok.vlan->parent, tok.vlan->pcp);
      vc.InterfaceConfig::name = name;
      vc.save();
      std::cout << "set interface: " << (exists ? "updated" : "created")
                << " vlan '" << name << "'\n";
      return;
    }

    if (effectiveType == InterfaceType::Tunnel ||
        effectiveType == InterfaceType::Gif ||
        effectiveType == InterfaceType::Tun) {
      TunnelConfig tc(base);
      tc.save();
      std::cout << "set interface: " << (exists ? "updated" : "created")
                << " tunnel '" << name << "'\n";
      return;
    }

    if (effectiveType == InterfaceType::Loopback) {
      LoopBackConfig lbc(base);
      lbc.save();
      std::cout << "set interface: " << (exists ? "updated" : "created")
                << " loopback '" << name << "'\n";
      return;
    }

    if (effectiveType == InterfaceType::Virtual) {
      VirtualInterfaceConfig vic(base);
      vic.save();
      std::cout << "set interface: " << (exists ? "updated" : "created")
                << " virtual iface '" << name << "'\n";
      return;
    }

    // If an address token was provided and the interface already exists,
    // add it as an alias rather than re-saving the whole interface.
    if (tok.address && exists) {
      auto net = IPNetwork::fromString(*tok.address);
      if (!net) {
        std::cerr << "set interface: invalid address '" << *tok.address
                  << "'\n";
        return;
      }
      if (net->family() == AddressFamily::IPv4) {
        auto v4 = dynamic_cast<IPv4Network *>(net.get());
        if (!v4) {
          std::cerr << "set interface: invalid IPv4 address\n";
          return;
        }
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) {
          std::cerr << "set interface: failed to create socket: "
                    << strerror(errno) << "\n";
          return;
        }
        struct ifaliasreq iar;
        std::memset(&iar, 0, sizeof(iar));
        std::strncpy(iar.ifra_name, name.c_str(), IFNAMSIZ - 1);
        struct sockaddr_in sa;
        std::memset(&sa, 0, sizeof(sa));
        sa.sin_len = sizeof(sa);
        sa.sin_family = AF_INET;
        auto netAddr = v4->address();
        auto v4addr = dynamic_cast<IPv4Address *>(netAddr.get());
        if (!v4addr) {
          std::cerr << "set interface: invalid IPv4 address object\n";
          close(sock);
          return;
        }
        sa.sin_addr.s_addr = htonl(v4addr->value());
        std::memcpy(&iar.ifra_addr, &sa, sizeof(sa));
        uint32_t maskval = (v4->mask() == 0) ? 0 : (~0u << (32 - v4->mask()));
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
          // fallback: try SIOCSIFADDR + SIOCSIFNETMASK (+ SIOCSIFDSTADDR for
          // /31)
          struct ifreq ifr;
          std::memset(&ifr, 0, sizeof(ifr));
          std::strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ - 1);
          std::memcpy(&ifr.ifr_addr, &sa, sizeof(sa));
          if (ioctl(sock, SIOCSIFADDR, &ifr) < 0) {
            std::cerr << "set interface: SIOCSIFADDR failed: "
                      << strerror(errno) << "\n";
            close(sock);
            return;
          }
          // Rely on SIOCAIFADDR/kernel behavior for netmask/peer handling.
          // Specific prefix handling removed from here to keep this path
          // generic.
          close(sock);
          std::cout << "set interface: added alias '" << *tok.address
                    << "' to '" << name << "'\n";
          return;
        }
        close(sock);
        std::cout << "set interface: added alias '" << *tok.address << "' to '"
                  << name << "'\n";
        return;
      } else {
        std::cerr << "set interface: IPv6 aliasing not yet implemented\n";
        return;
      }
    }

    // Default: apply generic interface updates
    base.save();
    std::cout << "set interface: " << (exists ? "updated" : "created")
              << " interface '" << name << "'\n";
  } catch (const std::exception &e) {
    std::cerr << "set interface: failed to create/update '" << name
              << "': " << e.what() << "\n";
  }
}
