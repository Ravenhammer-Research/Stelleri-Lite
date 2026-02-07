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

// FreeBSD LAGG configuration implementation moved out of the manager

// LAGG configuration implementation (moved from manager)

#include "LaggConfig.hpp"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <net/ethernet.h>
#include <net/if.h>
#include <net/if_lagg.h>
#include <stdexcept>
#include <string>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

LaggConfig::LaggConfig(const InterfaceConfig &base) {
  name = base.name;
  type = InterfaceType::Lagg;
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

LaggConfig::LaggConfig(const InterfaceConfig &base, LaggProtocol protocol_,
                       std::vector<std::string> members_,
                       std::optional<uint32_t> hash_policy_,
                       std::optional<int> lacp_rate_,
                       std::optional<int> min_links_) {
  name = base.name;
  type = InterfaceType::Lagg;
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

  protocol = protocol_;
  members = std::move(members_);
  hash_policy = hash_policy_;
  lacp_rate = lacp_rate_;
  min_links = min_links_;
}

void LaggConfig::save() const {
  if (name.empty())
    throw std::runtime_error("LaggConfig has no interface name set");

  if (!InterfaceConfig::exists(name)) {
    create();
  } else {
    InterfaceConfig::save();
  }

  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0) {
    throw std::runtime_error("Failed to create socket: " +
                             std::string(strerror(errno)));
  }

  int proto_value = 0;
  switch (protocol) {
  case LaggProtocol::FAILOVER:
    proto_value = 1;
    break;
  case LaggProtocol::LOADBALANCE:
    proto_value = 4;
    break;
  case LaggProtocol::LACP:
    proto_value = 3;
    break;
  case LaggProtocol::NONE:
    proto_value = 0;
    break;
  }

  if (proto_value > 0) {
    struct lagg_reqall ra;
    std::memset(&ra, 0, sizeof(ra));
    ra.ra_proto = proto_value;

    struct ifreq ifr;
    std::memset(&ifr, 0, sizeof(ifr));
    std::strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ - 1);
    ifr.ifr_data = reinterpret_cast<char *>(&ra);

    if (ioctl(sock, SIOCSLAGG, &ifr) < 0) {
      close(sock);
      throw std::runtime_error("Failed to set LAGG protocol: " +
                               std::string(strerror(errno)));
    }
  }

  for (const auto &member : members) {
    struct lagg_reqport rp;
    std::memset(&rp, 0, sizeof(rp));
    std::strncpy(rp.rp_portname, member.c_str(), IFNAMSIZ - 1);

    struct ifreq ifr;
    std::memset(&ifr, 0, sizeof(ifr));
    std::strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ - 1);
    ifr.ifr_data = reinterpret_cast<char *>(&rp);

    if (ioctl(sock, SIOCSLAGGPORT, &ifr) < 0) {
      close(sock);
      throw std::runtime_error("Failed to add port '" + member + "' to LAGG '" +
                               name + "': " + std::string(strerror(errno)));
    }
  }

  if (hash_policy) {
    std::cerr << "Note: Hash policy configuration for LAGG '" << name
              << "' may require sysctl settings\n";
  }

  if (lacp_rate) {
    std::cerr << "Note: LACP rate configuration for LAGG '" << name
              << "' may require per-port settings\n";
  }

  close(sock);
}

void LaggConfig::create() const {
  const std::string &nm = name;
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
