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
#include "TunnelConfig.hpp"
#include "VLANConfig.hpp"
#include "VirtualInterfaceConfig.hpp"
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/sockio.h>
#include <unistd.h>

namespace netcli {

void executeSetInterface(const InterfaceToken &tok,
                                 ConfigurationManager *mgr) {
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

    // Handle group assignment
    if (tok.group) {
      // Add group to base config if not already present
      bool has_group = false;
      for (const auto &g : base.groups) {
        if (g == *tok.group) {
          has_group = true;
          break;
        }
      }
      if (!has_group) {
        base.groups.push_back(*tok.group);
      }
    }

    // Handle MTU
    if (tok.mtu) {
      base.mtu = *tok.mtu;
    }

    // Handle status (up/down)
    if (tok.status) {
      if (base.flags) {
        if (*tok.status) {
          *base.flags |= IFF_UP;
        } else {
          *base.flags &= ~IFF_UP;
        }
      } else {
        // If no flags set yet, initialize with UP if requested
        base.flags = *tok.status ? IFF_UP : 0;
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
      // Handle tunnel-specific VRF if provided
      if (tok.tunnel_vrf) {
        if (!tc.vrf)
          tc.vrf = std::make_unique<VRFConfig>(*tok.tunnel_vrf);
        else
          tc.vrf->table = *tok.tunnel_vrf;
      }
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
      // Prefer to let InterfaceConfig / SystemConfigurationManager handle
      // adding aliases so system-specific ioctls remain in the System layer.
      base.aliases.emplace_back(net->clone());
      base.save();
      std::cout << "set interface: added alias '" << *tok.address << "' to '"
                << name << "'\n";
      return;
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
}
