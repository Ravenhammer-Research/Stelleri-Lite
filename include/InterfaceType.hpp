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
