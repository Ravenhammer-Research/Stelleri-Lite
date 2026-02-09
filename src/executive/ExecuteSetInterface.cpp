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
#include "CarpInterfaceConfig.hpp"
#include "ConfigurationManager.hpp"
#include "GreInterfaceConfig.hpp"
#include "IPAddress.hpp"
#include "IPNetwork.hpp"
#include "InterfaceConfig.hpp"
#include "InterfaceFlags.hpp"
#include "InterfaceToken.hpp"
#include "LaggInterfaceConfig.hpp"
#include "LoopBackInterfaceConfig.hpp"
#include "TunInterfaceConfig.hpp"
#include "GifInterfaceConfig.hpp"
#include "OvpnInterfaceConfig.hpp"
#include "IpsecInterfaceConfig.hpp"
#include "VlanInterfaceConfig.hpp"
#include "VxlanInterfaceConfig.hpp"
#include "EpairInterfaceConfig.hpp"
#include <iostream>

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
      bool exists = InterfaceConfig::exists(*mgr, name);
      auto ifopt = (exists && mgr) ? mgr->GetInterface(name)
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
      else if (base.name.rfind("gre", 0) == 0)
        effectiveType = InterfaceType::GRE;
      else if (base.name.rfind("vxlan", 0) == 0)
        effectiveType = InterfaceType::VXLAN;
      else if (base.name.rfind("carp", 0) == 0)
        effectiveType = InterfaceType::Virtual; // CARP uses virtual type

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
            *base.flags |= flagBit(InterfaceFlag::UP);
          } else {
            *base.flags &= ~flagBit(InterfaceFlag::UP);
          }
        } else {
          // If no flags set yet, initialize with UP if requested
          base.flags = *tok.status ? flagBit(InterfaceFlag::UP) : 0u;
        }
      }

      if (effectiveType == InterfaceType::Bridge) {
        BridgeInterfaceConfig bic(base);
        bic.save(*mgr);
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
        LaggInterfaceConfig lac(base, tok.lagg->protocol, tok.lagg->members,
                 tok.lagg->hash_policy, tok.lagg->lacp_rate,
                 tok.lagg->min_links);
        lac.save(*mgr);
        std::cout << "set interface: " << (exists ? "updated" : "created")
                  << " lagg '" << name << "'\n";
        return;
      }

      if (effectiveType == InterfaceType::VLAN) {
        if (!tok.vlan || tok.vlan->id == 0 || !tok.vlan->parent) {
          std::cerr
              << "set interface: VLAN creation requires VLAN id and parent "
                 "interface.\n"
              << "Usage: set interface name <vlan_name> vlan id <vlan_id> "
                 "parent <parent_iface>\n";
          return;
        }
        VlanInterfaceConfig vc(base, tok.vlan->id, tok.vlan->parent, tok.vlan->pcp);
        vc.InterfaceConfig::name = name;
        vc.save(*mgr);
        std::cout << "set interface: " << (exists ? "updated" : "created")
                  << " vlan '" << name << "'\n";
        return;
      }

      if (effectiveType == InterfaceType::Tun) {
        TunInterfaceConfig tc(base);
        if (tok.tunnel_vrf) {
          if (!tc.vrf)
            tc.vrf = std::make_unique<VRFConfig>(*tok.tunnel_vrf);
          else
            tc.vrf->table = *tok.tunnel_vrf;
        }
        tc.save(*mgr);
        std::cout << "set interface: " << (exists ? "updated" : "created")
                  << " tun '" << name << "'\n";
        return;
      }
      if (effectiveType == InterfaceType::Gif) {
        GifInterfaceConfig gc(base);
        if (tok.tunnel_vrf) {
          if (!gc.vrf)
            gc.vrf = std::make_unique<VRFConfig>(*tok.tunnel_vrf);
          else
            gc.vrf->table = *tok.tunnel_vrf;
        }
        gc.save(*mgr);
        std::cout << "set interface: " << (exists ? "updated" : "created")
                  << " gif '" << name << "'\n";
        return;
      }
      if (effectiveType == InterfaceType::IPsec) {
        IpsecInterfaceConfig icfg(base);
        if (tok.tunnel_vrf) {
          if (!icfg.vrf)
            icfg.vrf = std::make_unique<VRFConfig>(*tok.tunnel_vrf);
          else
            icfg.vrf->table = *tok.tunnel_vrf;
        }
        icfg.save(*mgr);
        std::cout << "set interface: " << (exists ? "updated" : "created")
                  << " ipsec '" << name << "'\n";
        return;
      }
      if (effectiveType == InterfaceType::Tunnel) {
        // Generic tunnel -> default to Tun
        TunInterfaceConfig tc(base);
        if (tok.tunnel_vrf) {
          if (!tc.vrf)
            tc.vrf = std::make_unique<VRFConfig>(*tok.tunnel_vrf);
          else
            tc.vrf->table = *tok.tunnel_vrf;
        }
        tc.save(*mgr);
        std::cout << "set interface: " << (exists ? "updated" : "created")
                  << " tunnel '" << name << "'\n";
        return;
      }

      if (effectiveType == InterfaceType::Loopback) {
        LoopBackInterfaceConfig lbc(base);
        lbc.save(*mgr);
        std::cout << "set interface: " << (exists ? "updated" : "created")
                  << " loopback '" << name << "'\n";
        return;
      }

      if (effectiveType == InterfaceType::GRE) {
        GreInterfaceConfig gc(base);
        gc.save(*mgr);
        std::cout << "set interface: " << (exists ? "updated" : "created")
                  << " gre '" << name << "'\n";
        return;
      }

      if (effectiveType == InterfaceType::VXLAN) {
        VxlanInterfaceConfig vxc(base);
        vxc.save(*mgr);
        std::cout << "set interface: " << (exists ? "updated" : "created")
                  << " vxlan '" << name << "'\n";
        return;
      }

      if (effectiveType == InterfaceType::Virtual) {
        VirtualInterfaceConfig vic(base);
        vic.save(*mgr);
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
        base.save(*mgr);
        std::cout << "set interface: added alias '" << *tok.address << "' to '"
                  << name << "'\n";
        return;
      }

      // Default: apply generic interface updates
      base.save(*mgr);
      std::cout << "set interface: " << (exists ? "updated" : "created")
                << " interface '" << name << "'\n";
    } catch (const std::exception &e) {
      std::cerr << "set interface: failed to create/update '" << name
                << "': " << e.what() << "\n";
    }
  }
} // namespace netcli
