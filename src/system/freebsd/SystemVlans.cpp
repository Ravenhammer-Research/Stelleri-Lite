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

#include "Socket.hpp"
#include "SystemConfigurationManager.hpp"
#include "VlanInterfaceConfig.hpp"

#include <cstring>
#include <net/if.h>
#include <net/if_vlan_var.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

std::vector<VlanInterfaceConfig> SystemConfigurationManager::GetVLANInterfaces(
    const std::vector<InterfaceConfig> &bases) const {
  std::vector<VlanInterfaceConfig> out;
  for (const auto &ic : bases) {
    if (ic.type == InterfaceType::VLAN) {
      VlanInterfaceConfig vconf(ic);

      Socket vsock(AF_INET, SOCK_DGRAM);
      struct vlanreq vreq{};
      struct ifreq ifr;
      prepare_ifreq(ifr, ic.name);
      ifr.ifr_data = reinterpret_cast<char *>(&vreq);

      if (ioctl(vsock, SIOCGETVLAN, &ifr) == 0) {
        vconf.id = static_cast<uint16_t>(vreq.vlr_tag & 0x0fff);
        int pcp = (vreq.vlr_tag >> 13) & 0x7;
        vconf.parent = std::string(vreq.vlr_parent);
        vconf.pcp = static_cast<PriorityCodePoint>(pcp);

        if (vreq.vlr_proto) {
          uint16_t proto = static_cast<uint16_t>(vreq.vlr_proto);
          if (proto == static_cast<uint16_t>(VLANProto::DOT1Q))
            vconf.proto = VLANProto::DOT1Q;
          else if (proto == static_cast<uint16_t>(VLANProto::DOT1AD))
            vconf.proto = VLANProto::DOT1AD;
          else
            vconf.proto = VLANProto::OTHER;
        }
      }

      auto query_caps =
          [&](const std::string &name) -> std::optional<uint32_t> {
        Socket csock(AF_INET, SOCK_DGRAM);
        struct ifreq cifr;
        prepare_ifreq(cifr, name);
        if (ioctl(csock, SIOCGIFCAP, &cifr) == 0) {
          unsigned int curcap = cifr.ifr_curcap;
          return static_cast<uint32_t>(curcap);
        }
        return std::nullopt;
      };

      if (auto o = query_caps(ic.name); o) {
        vconf.options_bits = *o;
      } else if (vconf.parent) {
        if (auto o = query_caps(*vconf.parent); o) {
          vconf.options_bits = *o;
        }
      }

      out.emplace_back(std::move(vconf));
    }
  }
  return out;
}

void SystemConfigurationManager::SaveVlan(
    const VlanInterfaceConfig &vlan) const {
  if (vlan.name.empty())
    throw std::runtime_error("VlanInterfaceConfig has no interface name set");

  if (!vlan.parent || vlan.id == 0) {
    throw std::runtime_error(
        "VLAN configuration requires parent interface and VLAN ID");
  }

  if (!InterfaceConfig::exists(*this, vlan.name)) {
    CreateInterface(vlan.name);
  } else {
    SaveInterface(InterfaceConfig(vlan));
  }

#if defined(SIOCSETVLAN)
  Socket vsock(AF_INET, SOCK_DGRAM);

  struct vlanreq vreq{};
  std::strncpy(vreq.vlr_parent, vlan.parent->c_str(), IFNAMSIZ - 1);
  vreq.vlr_tag = vlan.id;
  if (vlan.pcp) {
    vreq.vlr_tag |= (static_cast<int>(*vlan.pcp) & 0x7) << 13;
  }

  struct ifreq ifr;
  prepare_ifreq(ifr, vlan.name);
  ifr.ifr_data = reinterpret_cast<char *>(&vreq);

  if (ioctl(vsock, SIOCSETVLAN, &ifr) < 0) {
    throw std::runtime_error("Failed to configure VLAN: " +
                             std::string(strerror(errno)));
  }
#else
  throw std::runtime_error("VLAN configuration not supported on this platform");
#endif
}
