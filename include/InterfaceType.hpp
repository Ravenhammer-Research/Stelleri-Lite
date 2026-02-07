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

/**
 * @file InterfaceType.hpp
 * @brief Network interface type enumeration
 */

#pragma once

#include <ifaddrs.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <string>

/**
 * @brief Network interface types
 *
 * Covers common physical and virtual interface types.
 */
enum class InterfaceType {
  Unknown,
  Loopback,
  Ethernet,
  PointToPoint,
  Wireless,
  Bridge,
  Lagg,
  VLAN,
  PPP,
  Tunnel,
  Gif,
  Tun,
  FDDI,
  TokenRing,
  ATM,
  Virtual,
  Other,
};

// `detectInterfaceType` moved to the system implementation where `struct
// ifaddrs` is available so link-layer address families can be examined
// per-platform.

inline InterfaceType ifAddrToInterfaceType(const struct ifaddrs *ifa) {
  if (!ifa)
    return InterfaceType::Unknown;
  unsigned int flags = ifa->ifa_flags;

  // Inspect link-layer sockaddr (AF_LINK / sockaddr_dl) on BSD-like systems
  // first so we can detect specific virtual link types (gif/tun/bridge)
  // even when point-to-point or other flags are set.
  if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_LINK) {
    struct sockaddr_dl *sdl =
        reinterpret_cast<struct sockaddr_dl *>(ifa->ifa_addr);
    switch (sdl->sdl_type) {
    case IFT_ETHER:
    case IFT_FASTETHER:
    case IFT_GIGABITETHERNET:
    case IFT_FIBRECHANNEL:
    case IFT_AFLANE8023:
      return InterfaceType::Ethernet;
    case IFT_IEEE8023ADLAG:
      return InterfaceType::Lagg;
    case IFT_LOOP:
      return InterfaceType::Loopback;
    case IFT_PPP:
      return InterfaceType::PPP;
    case IFT_TUNNEL:
      return InterfaceType::Tunnel;
    case IFT_GIF:
      return InterfaceType::Gif;
    case IFT_FDDI:
      return InterfaceType::FDDI;
    case IFT_ISO88025: /* token ring / token bus family */
    case IFT_ISO88023:
    case IFT_ISO88024:
    case IFT_ISO88026:
      return InterfaceType::TokenRing;
    case IFT_IEEE80211:
      return InterfaceType::Wireless;
    case IFT_BRIDGE:
      return InterfaceType::Bridge;
    case IFT_L2VLAN:
      return InterfaceType::VLAN;
    case IFT_ATM:
      return InterfaceType::ATM;
    case IFT_PROPVIRTUAL:
    case IFT_VIRTUALIPADDRESS:
      return InterfaceType::Virtual;
    default:
      return InterfaceType::Other;
    }
  }
  // Fall back to flag-based classification when link-layer type is not
  // informative.
  if (flags & IFF_LOOPBACK)
    return InterfaceType::Loopback;
  if (flags & IFF_POINTOPOINT)
    return InterfaceType::PointToPoint;

  return InterfaceType::Unknown;
}
