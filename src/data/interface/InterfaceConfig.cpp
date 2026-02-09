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
#include "ConfigurationManager.hpp"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <optional>

#include "IPAddress.hpp"
#include "IPNetwork.hpp"

#include "BridgeTableFormatter.hpp"
#include "CarpTableFormatter.hpp"
#include "GreTableFormatter.hpp"
#include "InterfaceTableFormatter.hpp"
#include "LaggTableFormatter.hpp"
#include "SixToFourTableFormatter.hpp"
#include "TapTableFormatter.hpp"
#include "TunTableFormatter.hpp"
#include "GifTableFormatter.hpp"
#include "OvpnTableFormatter.hpp"
#include "IpsecTableFormatter.hpp"
#include "VlanTableFormatter.hpp"
#include "VRFConfig.hpp"
#include "VxlanTableFormatter.hpp"
#include "EpairTableFormatter.hpp"
#include "WlanTableFormatter.hpp"

// (no helpers) prepare `ifreq` inline where needed

// Platform-specific `struct ifaddrs` constructor removed. System layer
// (`SystemConfigurationManager`) is responsible for enumerating interfaces
// and constructing `InterfaceConfig` instances.

InterfaceConfig::InterfaceConfig(
    std::string name_, InterfaceType type_, std::unique_ptr<IPNetwork> address_,
    std::vector<std::unique_ptr<IPNetwork>> aliases_,
    std::unique_ptr<VRFConfig> vrf_, std::optional<uint32_t> flags_,
    std::vector<std::string> groups_, std::optional<int> mtu_)
    : name(std::move(name_)), type(type_), address(std::move(address_)),
      aliases(std::move(aliases_)), vrf(std::move(vrf_)), flags(flags_),
      groups(std::move(groups_)), mtu(mtu_) {}

void InterfaceConfig::save(ConfigurationManager &mgr) const {
  mgr.SaveInterface(*this);
}

// Interface existence helper is provided by `ConfigData::exists`

void InterfaceConfig::destroy(ConfigurationManager &mgr) const {
  mgr.DestroyInterface(name);
}

void InterfaceConfig::removeAddress(ConfigurationManager &mgr,
                                    const std::string &addr) const {
  mgr.RemoveInterfaceAddress(name, addr);
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
  metric = o.metric;
  nd6_options = o.nd6_options;
  description = o.description;
  hwaddr = o.hwaddr;
  capabilities = o.capabilities;
  req_capabilities = o.req_capabilities;
  media = o.media;
  media_active = o.media_active;
  media_status = o.media_status;
  status_str = o.status_str;
  phys = o.phys;
  baudrate = o.baudrate;
  link_state = o.link_state;
}

// Static helper: check whether a named interface exists.
bool InterfaceConfig::exists(const ConfigurationManager &mgr,
                             std::string_view name) {
  return mgr.InterfaceExists(name);
}

// Type checking predicates
bool InterfaceConfig::isBridge() const { return type == InterfaceType::Bridge; }

bool InterfaceConfig::isLagg() const { return type == InterfaceType::Lagg; }

bool InterfaceConfig::isVlan() const { return type == InterfaceType::VLAN; }

bool InterfaceConfig::isTunnelish() const {
  return type == InterfaceType::Tunnel || type == InterfaceType::Gif ||
         type == InterfaceType::Tun;
}

bool InterfaceConfig::isVirtual() const {
  return type == InterfaceType::Virtual;
}

bool InterfaceConfig::isWlan() const { return type == InterfaceType::Wireless; }

bool InterfaceConfig::isSixToFour() const {
  if (!isTunnelish())
    return false;
  return name.rfind("gif", 0) == 0 || name.rfind("stf", 0) == 0 ||
         name.rfind("sit", 0) == 0;
}

bool InterfaceConfig::isTap() const {
  return isVirtual() || name.rfind("tap", 0) == 0;
}

bool InterfaceConfig::isCarp() const {
  return name.rfind("carp", 0) == 0 || name.rfind("vh", 0) == 0;
}

bool InterfaceConfig::isGre() const {
  return type == InterfaceType::GRE || name.rfind("gre", 0) == 0;
}

bool InterfaceConfig::isVxlan() const {
  return type == InterfaceType::VXLAN || name.rfind("vxlan", 0) == 0;
}

bool InterfaceConfig::isIpsec() const {
  return type == InterfaceType::IPsec || name.rfind("ipsec", 0) == 0;
}

// Check if this interface matches a requested type (handles tunnel special
// cases)
bool InterfaceConfig::matchesType(InterfaceType requestedType) const {
  // Special handling for tunnel-ish types: they all match each other
  if (requestedType == InterfaceType::Tunnel ||
      requestedType == InterfaceType::Gif ||
      requestedType == InterfaceType::Tun) {
    return isTunnelish();
  }
  return type == requestedType;
}

// Format a collection of interfaces using the appropriate formatter
std::string
InterfaceConfig::formatInterfaces(const std::vector<InterfaceConfig> &ifaces,
                                  ConfigurationManager *mgr) {
  if (ifaces.empty())
    return "No interfaces found.\n";

  // Determine which specialized formatter to use based on the collection
  bool allSame = true;
  auto checkType = ifaces[0].type;

  for (const auto &iface : ifaces) {
    if (iface.type != checkType) {
      allSame = false;
      break;
    }
  }

  if (!allSame) {
    // Mixed types - use generic formatter
    InterfaceTableFormatter formatter;
    return formatter.format(ifaces);
  }

  // All same type - check which specialized formatter to use
  if (ifaces[0].isBridge()) {
    BridgeTableFormatter formatter(mgr);
    return formatter.format(ifaces);
  }
  if (ifaces[0].isLagg()) {
    LaggTableFormatter formatter;
    return formatter.format(ifaces);
  }
  if (ifaces[0].isVlan()) {
    VlanTableFormatter formatter;
    return formatter.format(ifaces);
  }
  if (ifaces[0].isWlan()) {
    WlanTableFormatter formatter;
    return formatter.format(ifaces);
  }
  if (ifaces[0].isSixToFour()) {
    SixToFourTableFormatter formatter;
    return formatter.format(ifaces);
  }
  if (ifaces[0].isTunnelish()) {
    // Choose specific tunnel formatter by concrete type or name heuristics
    auto t = ifaces[0].type;
    if (t == InterfaceType::Gif) {
      GifTableFormatter formatter;
      return formatter.format(ifaces);
    }
    if (t == InterfaceType::Tun) {
      TunTableFormatter formatter;
      return formatter.format(ifaces);
    }
    if (t == InterfaceType::IPsec) {
      IpsecTableFormatter formatter;
      return formatter.format(ifaces);
    }
    // Heuristic: if name prefix 'ovpn' use Ovpn formatter
    if (ifaces[0].name.rfind("ovpn", 0) == 0) {
      OvpnTableFormatter formatter;
      return formatter.format(ifaces);
    }
    // Default to Tun formatter for generic tunnel type
    TunTableFormatter formatter;
    return formatter.format(ifaces);
  }
  if (ifaces[0].isGre()) {
    GRETableFormatter formatter;
    return formatter.format(ifaces);
  }
  if (ifaces[0].isVxlan()) {
    VxlanTableFormatter formatter;
    return formatter.format(ifaces);
  }
  if (ifaces[0].isTap()) {
    TapTableFormatter formatter;
    return formatter.format(ifaces);
  }
  if (ifaces[0].isCarp()) {
    CarpTableFormatter formatter;
    return formatter.format(ifaces);
  }
  if (ifaces[0].isVirtual()) {
    VirtualTableFormatter formatter;
    return formatter.format(ifaces);
  }

  // Default to generic formatter
  InterfaceTableFormatter formatter;
  return formatter.format(ifaces);
}
