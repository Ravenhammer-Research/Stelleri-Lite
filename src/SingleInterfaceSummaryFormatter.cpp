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

#include "SingleInterfaceSummaryFormatter.hpp"
#include "BridgeInterfaceConfig.hpp"
#include "InterfaceConfig.hpp"
#include "InterfaceFlags.hpp"
#include "InterfaceType.hpp"
#include "LaggConfig.hpp"
#include "TunnelConfig.hpp"
#include "VLANConfig.hpp"
#include <sstream>

std::string
SingleInterfaceSummaryFormatter::format(const ConfigData &cd) const {
  if (!cd.iface)
    return "Interface not found.\n";

  const auto &ic = *cd.iface;
  std::ostringstream oss;

  oss << "Interface: " << ic.name << "\n";
  oss << "Type:      " << std::to_string(static_cast<int>(ic.type)) << "\n";

  if (ic.flags) {
    std::string status = "-";
    if (*ic.flags & static_cast<uint32_t>(InterfaceFlag::RUNNING)) {
      status = "active";
    } else if (*ic.flags & static_cast<uint32_t>(InterfaceFlag::UP)) {
      status = "no-carrier";
    } else {
      status = "down";
    }
    oss << "Status:    " << status << "\n";
  }

  if (ic.mtu) {
    oss << "MTU:       " << *ic.mtu << "\n";
  }

  if (ic.address) {
    oss << "Address:   " << ic.address->toString() << "\n";
  }

  for (const auto &alias : ic.aliases) {
    oss << "           " << alias->toString() << "\n";
  }

  if (ic.vrf) {
    oss << "VRF:       " << ic.vrf->name << "\n";
  }

  // Tunnel details (use dynamic_cast to access derived TunnelConfig)
  if (auto tptr = dynamic_cast<const TunnelConfig *>(&ic)) {
    if (tptr->tunnel_vrf)
      oss << "Tunnel VRF: " << *tptr->tunnel_vrf << "\n";
    if (tptr->source)
      oss << "Tunnel Src: " << tptr->source->toString() << "\n";
    if (tptr->destination)
      oss << "Tunnel Dst: " << tptr->destination->toString() << "\n";
  }

  // Show bridge details
  if (auto bptr = dynamic_cast<const BridgeInterfaceConfig *>(&ic)) {
    oss << "Bridge STP: " << (bptr->stp ? "enabled" : "disabled") << "\n";
    if (!bptr->members.empty()) {
      oss << "Members:   ";
      bool first = true;
      for (const auto &m : bptr->members) {
        if (!first)
          oss << ", ";
        oss << m;
        first = false;
      }
      oss << "\n";
    }
  }

  // Show LAGG details
  if (auto lptr = dynamic_cast<const LaggConfig *>(&ic)) {
    oss << "LAGG Proto: ";
    switch (lptr->protocol) {
    case LaggProtocol::LACP:
      oss << "LACP";
      break;
    case LaggProtocol::FAILOVER:
      oss << "Failover";
      break;
    case LaggProtocol::LOADBALANCE:
      oss << "Load Balance";
      break;
    case LaggProtocol::NONE:
      oss << "None";
      break;
    }
    oss << "\n";

    if (!lptr->members.empty()) {
      oss << "Members:   ";
      bool first = true;
      for (const auto &m : lptr->members) {
        if (!first)
          oss << ", ";
        oss << m;
        first = false;
      }
      oss << "\n";
    }
  }

  // Show VLAN details
  if (auto vptr = dynamic_cast<const VLANConfig *>(&ic)) {
    oss << "VLAN ID:   " << vptr->id << "\n";
    if (vptr->parent)
      oss << "Parent:    " << *vptr->parent << "\n";
    if (vptr->pcp)
      oss << "PCP:       " << static_cast<int>(*vptr->pcp) << "\n";
  }

  if (ic.flags) {
    oss << "Flags:     ";
    if (*ic.flags & static_cast<uint32_t>(InterfaceFlag::UP))
      oss << "UP ";
    if (*ic.flags & static_cast<uint32_t>(InterfaceFlag::BROADCAST))
      oss << "BROADCAST ";
    if (*ic.flags & static_cast<uint32_t>(InterfaceFlag::LOOPBACK))
      oss << "LOOPBACK ";
    if (*ic.flags & static_cast<uint32_t>(InterfaceFlag::POINTOPOINT))
      oss << "POINTOPOINT ";
    if (*ic.flags & static_cast<uint32_t>(InterfaceFlag::RUNNING))
      oss << "RUNNING ";
    if (*ic.flags & static_cast<uint32_t>(InterfaceFlag::MULTICAST))
      oss << "MULTICAST";
    oss << "\n";
  }

  return oss.str();
}
