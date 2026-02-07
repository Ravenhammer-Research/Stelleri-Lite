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

#include "VLANConfig.hpp"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <net/if.h>
#include <net/if_vlan_var.h>
#include <stdexcept>
#include <string>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

VLANConfig::VLANConfig(const InterfaceConfig &base) {
  name = base.name;
  type = base.type;
  if (base.address)
    address = base.address->clone();
  aliases.clear();
  for (const auto &a : base.aliases) {
    if (a)
      aliases.push_back(a->clone());
  }
  if (base.vrf)
    vrf = std::make_unique<VRFConfig>(*base.vrf);
  flags = base.flags;
  groups = base.groups;
  mtu = base.mtu;
}

VLANConfig::VLANConfig(const InterfaceConfig &base, uint16_t id_,
                       std::optional<std::string> name_,
                       std::optional<std::string> parent_,
                       std::optional<PriorityCodePoint> pcp_) {
  name = base.name;
  type = base.type;
  if (base.address)
    address = base.address->clone();
  aliases.clear();
  for (const auto &a : base.aliases) {
    if (a)
      aliases.push_back(a->clone());
  }
  if (base.vrf)
    vrf = std::make_unique<VRFConfig>(*base.vrf);
  flags = base.flags;
  groups = base.groups;
  mtu = base.mtu;

  id = id_;
  name = name_ ? *name_ : name;
  parent = parent_;
  pcp = pcp_;
}

void VLANConfig::save() const {
  if (InterfaceConfig::name.empty())
    throw std::runtime_error("VLANConfig has no interface name set");

  if (!parent || id == 0) {
    throw std::runtime_error(
        "VLAN configuration requires parent interface and VLAN ID");
  }
  if (!InterfaceConfig::exists(InterfaceConfig::name)) {
    create();
  } else {
    InterfaceConfig::save();
  }

#if defined(SIOCSETVLAN)
  int vsock = socket(AF_INET, SOCK_DGRAM, 0);
  if (vsock < 0) {
    throw std::runtime_error("Failed to create socket: " +
                             std::string(strerror(errno)));
  }

  struct vlanreq vreq;
  std::memset(&vreq, 0, sizeof(vreq));
  std::strncpy(vreq.vlr_parent, parent->c_str(), IFNAMSIZ - 1);
  vreq.vlr_tag = id;
  if (pcp) {
    vreq.vlr_tag |= (static_cast<int>(*pcp) & 0x7) << 13;
  }

  struct ifreq ifr;
  std::memset(&ifr, 0, sizeof(ifr));
  std::strncpy(ifr.ifr_name, InterfaceConfig::name.c_str(), IFNAMSIZ - 1);
  ifr.ifr_data = reinterpret_cast<char *>(&vreq);

  if (ioctl(vsock, SIOCSETVLAN, &ifr) < 0) {
    close(vsock);
    throw std::runtime_error("Failed to configure VLAN: " +
                             std::string(strerror(errno)));
  }

  close(vsock);
#else
  throw std::runtime_error("VLAN configuration not supported on this platform");
#endif
}

void VLANConfig::create() const {
  const std::string &nm = InterfaceConfig::name;
  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0) {
    throw std::runtime_error("Failed to create socket: " +
                             std::string(strerror(errno)));
  }

  struct ifreq ifr;
  std::memset(&ifr, 0, sizeof(ifr));
  std::strncpy(ifr.ifr_name, nm.c_str(), IFNAMSIZ - 1);

  if (ioctl(sock, SIOCIFCREATE, &ifr) < 0) {
    close(sock);
    throw std::runtime_error("Failed to create interface '" + nm +
                             "': " + std::string(strerror(errno)));
  }

  close(sock);
}
