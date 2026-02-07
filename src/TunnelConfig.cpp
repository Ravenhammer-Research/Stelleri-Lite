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

#include "TunnelConfig.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <net/if.h>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

TunnelConfig::TunnelConfig(const InterfaceConfig &base) {
  name = base.name;
  InterfaceConfig::type = base.type;
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

TunnelConfig::TunnelConfig(const InterfaceConfig &base, TunnelType type_,
                           std::unique_ptr<IPAddress> source_,
                           std::unique_ptr<IPAddress> destination_) {
  name = base.name;
  InterfaceConfig::type = base.type;
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

  type = type_;
  source = std::move(source_);
  destination = std::move(destination_);
}

void TunnelConfig::save() const {
  if (name.empty())
    throw std::runtime_error("TunnelConfig has no interface name set");

  if (!source || !destination) {
    throw std::runtime_error("Tunnel endpoints not configured");
  }

  if (!InterfaceConfig::exists(name)) {
    create();
  } else {
    InterfaceConfig::save();
  }

  auto src_addr = IPAddress::fromString(source->toString());
  auto dst_addr = IPAddress::fromString(destination->toString());
  if (!src_addr || !dst_addr) {
    throw std::runtime_error("Invalid tunnel endpoint addresses");
  }
  if (src_addr->family() != dst_addr->family()) {
    throw std::runtime_error(
        "Tunnel endpoints must be same address family (both IPv4 or IPv6)");
  }

  int tsock = socket(AF_INET, SOCK_DGRAM, 0);
  if (tsock < 0) {
    throw std::runtime_error("Failed to create socket: " +
                             std::string(strerror(errno)));
  }

  struct ifaliasreq ifra;
  std::memset(&ifra, 0, sizeof(ifra));
  std::strncpy(ifra.ifra_name, name.c_str(), IFNAMSIZ - 1);

  if (src_addr->family() == AddressFamily::IPv4) {
    auto v4src = dynamic_cast<const IPv4Address *>(src_addr.get());
    auto v4dst = dynamic_cast<const IPv4Address *>(dst_addr.get());

    struct sockaddr_in *sin_src =
        reinterpret_cast<struct sockaddr_in *>(&ifra.ifra_addr);
    sin_src->sin_family = AF_INET;
    sin_src->sin_len = sizeof(struct sockaddr_in);
    sin_src->sin_addr.s_addr = htonl(v4src->value());

    struct sockaddr_in *sin_dst =
        reinterpret_cast<struct sockaddr_in *>(&ifra.ifra_broadaddr);
    sin_dst->sin_family = AF_INET;
    sin_dst->sin_len = sizeof(struct sockaddr_in);
    sin_dst->sin_addr.s_addr = htonl(v4dst->value());

    if (ioctl(tsock, SIOCSIFPHYADDR, &ifra) < 0) {
      close(tsock);
      throw std::runtime_error("Failed to configure GIF tunnel endpoints: " +
                               std::string(strerror(errno)));
    }
  } else {
    std::cerr << "Warning: IPv6 tunnel configuration requires routing socket - "
                 "not fully implemented\n";
  }

  (void)0;

  close(tsock);
}

void TunnelConfig::create() const {
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
