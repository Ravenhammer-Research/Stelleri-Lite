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
 * @file InterfaceFlags.hpp
 * @brief Interface flag definitions
 */

#pragma once

#include <cstdint>
#include <string>

/**
 * @brief FreeBSD interface flags
 *
 * Matches FreeBSD's net/if.h IFF_* constants.
 */
enum class InterfaceFlag : uint32_t {
  UP = 0x1,             ///< Interface is up (IFF_UP)
  BROADCAST = 0x2,      ///< Broadcast address valid (IFF_BROADCAST)
  DEBUG = 0x4,          ///< Turn on debugging (IFF_DEBUG)
  LOOPBACK = 0x8,       ///< Is a loopback interface (IFF_LOOPBACK)
  POINTOPOINT = 0x10,   ///< Point-to-point link (IFF_POINTOPOINT)
  NEEDSEPOCH = 0x20,    ///< Calls if_input w/o net epoch (IFF_NEEDSEPOCH)
  RUNNING = 0x40,       ///< Resources allocated / running (IFF_DRV_RUNNING)
  NOARP = 0x80,         ///< No address resolution protocol (IFF_NOARP)
  PROMISC = 0x100,      ///< Receive all packets (IFF_PROMISC)
  ALLMULTI = 0x200,     ///< Receive all multicast packets (IFF_ALLMULTI)
  OACTIVE = 0x400,      ///< Tx hardware queue is full (IFF_DRV_OACTIVE)
  SIMPLEX = 0x800,      ///< Can't hear own transmissions (IFF_SIMPLEX)
  LINK0 = 0x1000,       ///< Link-layer defined bit 0 (IFF_LINK0)
  LINK1 = 0x2000,       ///< Link-layer defined bit 1 (IFF_LINK1)
  LINK2 = 0x4000,       ///< Link-layer defined bit 2 (IFF_LINK2)
  ALTPHYS = LINK2,      ///< Use alternate physical connection (IFF_ALTPHYS)
  MULTICAST = 0x8000,   ///< Supports multicast (IFF_MULTICAST)
  CANTCONFIG = 0x10000, ///< Unconfigurable using ioctl(2) (IFF_CANTCONFIG)
  PPROMISC = 0x20000,   ///< User-requested promisc mode (IFF_PPROMISC)
  MONITOR = 0x40000,    ///< User-requested monitor mode (IFF_MONITOR)
  STATICARP = 0x80000,  ///< Static ARP (IFF_STATICARP)
  STICKYARP = 0x100000, ///< Sticky ARP (IFF_STICKYARP)
  DYING = 0x200000,     ///< Interface is winding down (IFF_DYING)
  RENAMING = 0x400000,  ///< Interface is being renamed (IFF_RENAMING)
  PALLMULTI = 0x800000, ///< User-requested allmulti mode (IFF_PALLMULTI)
  NETLINK_1 = 0x1000000 ///< Used by netlink (IFF_NETLINK_1)
};

/// @brief Test whether a raw flags bitmask contains a particular InterfaceFlag.
constexpr bool hasFlag(uint32_t flags, InterfaceFlag f) noexcept {
  return (flags & static_cast<uint32_t>(f)) != 0;
}

/// @brief Return the underlying uint32_t value of an InterfaceFlag.
constexpr uint32_t flagBit(InterfaceFlag f) noexcept {
  return static_cast<uint32_t>(f);
}

/**
 * @brief Convert interface flags bitmask to human-readable string
 * @param flags Bitmask of interface flags
 * @return String representation (e.g., "UBRM")
 */
inline std::string flagsToString(uint32_t flags) {
  std::string result;
  if (flags & static_cast<uint32_t>(InterfaceFlag::UP))
    result += "U";
  if (flags & static_cast<uint32_t>(InterfaceFlag::BROADCAST))
    result += "B";
  if (flags & static_cast<uint32_t>(InterfaceFlag::DEBUG))
    result += "D";
  if (flags & static_cast<uint32_t>(InterfaceFlag::LOOPBACK))
    result += "L";
  if (flags & static_cast<uint32_t>(InterfaceFlag::POINTOPOINT))
    result += "P";
  if (flags & static_cast<uint32_t>(InterfaceFlag::RUNNING))
    result += "R";
  if (flags & static_cast<uint32_t>(InterfaceFlag::NOARP))
    result += "N";
  if (flags & static_cast<uint32_t>(InterfaceFlag::PROMISC))
    result += "O";
  if (flags & static_cast<uint32_t>(InterfaceFlag::ALLMULTI))
    result += "A";
  if (flags & static_cast<uint32_t>(InterfaceFlag::MULTICAST))
    result += "M";
  // Extended flags (use lowercase or digits to avoid colliding with
  // the historical single-letter mappings above)
  if (flags & static_cast<uint32_t>(InterfaceFlag::NEEDSEPOCH))
    result += "e"; // NEEDSEPOCH
  if (flags & static_cast<uint32_t>(InterfaceFlag::OACTIVE))
    result += "q"; // driver oactive (queue full)
  if (flags & static_cast<uint32_t>(InterfaceFlag::SIMPLEX))
    result += "s"; // SIMPLEX
  if (flags & static_cast<uint32_t>(InterfaceFlag::LINK0))
    result += "0"; // LINK0
  if (flags & static_cast<uint32_t>(InterfaceFlag::LINK1))
    result += "1"; // LINK1
  if (flags & static_cast<uint32_t>(InterfaceFlag::LINK2))
    result += "2"; // LINK2 (also ALTPHYS)
  if (flags & static_cast<uint32_t>(InterfaceFlag::CANTCONFIG))
    result += "C"; // CANTCONFIG
  if (flags & static_cast<uint32_t>(InterfaceFlag::PPROMISC))
    result += "p"; // PPROMISC (user promisc)
  if (flags & static_cast<uint32_t>(InterfaceFlag::MONITOR))
    result += "m"; // MONITOR
  if (flags & static_cast<uint32_t>(InterfaceFlag::STATICARP))
    result += "t"; // STATICARP
  if (flags & static_cast<uint32_t>(InterfaceFlag::STICKYARP))
    result += "k"; // STICKYARP
  if (flags & static_cast<uint32_t>(InterfaceFlag::DYING))
    result += "x"; // DYING
  if (flags & static_cast<uint32_t>(InterfaceFlag::RENAMING))
    result += "z"; // RENAMING
  if (flags & static_cast<uint32_t>(InterfaceFlag::PALLMULTI))
    result += "a"; // PALLMULTI (user allmulti)
  if (flags & static_cast<uint32_t>(InterfaceFlag::NETLINK_1))
    result += "l"; // NETLINK_1
  return result.empty() ? "-" : result;
}
