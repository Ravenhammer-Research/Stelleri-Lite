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

#include "SystemConfigurationManager.hpp"
#include "VlanInterfaceConfig.hpp"
#include <cstring>
#include <linux/if_vlan.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

std::vector<VlanInterfaceConfig> SystemConfigurationManager::GetVLANInterfaces(
    const std::vector<InterfaceConfig> &bases) const {
  std::vector<VlanInterfaceConfig> out;
  for (const auto &ic : bases) {
    if (ic.type == InterfaceType::VLAN) {
      VlanInterfaceConfig vconf(ic);
      // Linux VLAN info is often retrieved via netlink or /proc/net/vlan/config
      // For now, we'll use a basic implementation or stub as ioctls for
      // GET_VLAN_REALDEV_NAME and GET_VLAN_VID exist.
      int sock = socket(AF_INET, SOCK_DGRAM, 0);
      if (sock >= 0) {
        struct vlan_ioctl_args vlan_args{};
        vlan_args.cmd = GET_VLAN_VID_CMD;
        std::strncpy(vlan_args.device1, ic.name.c_str(),
                     sizeof(vlan_args.device1) - 1);
        if (ioctl(sock, SIOCGIFVLAN, &vlan_args) == 0) {
          vconf.id = vlan_args.u.VID;
        }

        std::memset(&vlan_args, 0, sizeof(vlan_args));
        vlan_args.cmd = GET_VLAN_REALDEV_NAME_CMD;
        std::strncpy(vlan_args.device1, ic.name.c_str(),
                     sizeof(vlan_args.device1) - 1);
        if (ioctl(sock, SIOCGIFVLAN, &vlan_args) == 0) {
          vconf.parent = std::string(vlan_args.u.device2);
        }
        close(sock);
      }
      out.emplace_back(std::move(vconf));
    }
  }
  return out;
}

void SystemConfigurationManager::SaveVlan(const VlanInterfaceConfig &vlan
                                          [[maybe_unused]]) const {
  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0)
    return;

  struct vlan_ioctl_args vlan_args{};
  vlan_args.cmd = ADD_VLAN_CMD;
  if (vlan.parent) {
    std::strncpy(vlan_args.device1, vlan.parent->c_str(),
                 sizeof(vlan_args.device1) - 1);
  }
  vlan_args.u.VID = vlan.id;

  ioctl(sock, SIOCSIFVLAN, &vlan_args);
  close(sock);
}
