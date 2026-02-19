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
  GRE,
  VXLAN,
  IPsec,
  FDDI,
  TokenRing,
  ATM,
  Epair,
  Carp,
  Tap,
  SixToFour,
  Ovpn,
  Pflog,
  Pfsync,
  WireGuard,
  Enc,
  Other,
};

/// Convert an InterfaceType enum value to a human-readable string.
inline std::string interfaceTypeToString(InterfaceType t) {
  switch (t) {
  case InterfaceType::Unknown:
    return "Unknown";
  case InterfaceType::Loopback:
    return "Loopback";
  case InterfaceType::Ethernet:
    return "Ethernet";
  case InterfaceType::PointToPoint:
    return "PointToPoint";
  case InterfaceType::Wireless:
    return "Wireless";
  case InterfaceType::Bridge:
    return "Bridge";
  case InterfaceType::Lagg:
    return "LinkAggregate";
  case InterfaceType::VLAN:
    return "VLAN";
  case InterfaceType::PPP:
    return "PPP";
  case InterfaceType::Tunnel:
    return "Tunnel";
  case InterfaceType::Gif:
    return "GenericTunnel";
  case InterfaceType::Tun:
    return "Tun";
  case InterfaceType::GRE:
    return "GRE";
  case InterfaceType::VXLAN:
    return "VXLAN";
  case InterfaceType::IPsec:
    return "IPsec";
  case InterfaceType::FDDI:
    return "FDDI";
  case InterfaceType::TokenRing:
    return "TokenRing";
  case InterfaceType::ATM:
    return "ATM";
  case InterfaceType::Epair:
    return "Epair";
  case InterfaceType::Carp:
    return "Carp";
  case InterfaceType::Tap:
    return "Tap";
  case InterfaceType::SixToFour:
    return "SixToFour";
  case InterfaceType::Ovpn:
    return "OpenVPN";
  case InterfaceType::Pflog:
    return "Pflog";
  case InterfaceType::Pfsync:
    return "Pfsync";
  case InterfaceType::WireGuard:
    return "WireGuard";
  case InterfaceType::Enc:
    return "Enc";
  case InterfaceType::Other:
    return "Other";
  default:
    return "Unknown";
  }
}

/// Parse a CLI type keyword (e.g. "ethernet", "bridge") into InterfaceType.
inline InterfaceType interfaceTypeFromString(const std::string &s) {
  if (s == "ethernet")
    return InterfaceType::Ethernet;
  if (s == "loopback")
    return InterfaceType::Loopback;
  if (s == "ppp")
    return InterfaceType::PPP;
  if (s == "bridge")
    return InterfaceType::Bridge;
  if (s == "vlan")
    return InterfaceType::VLAN;
  if (s == "lagg" || s == "lag")
    return InterfaceType::Lagg;
  if (s == "tunnel")
    return InterfaceType::Tunnel;
  if (s == "gif")
    return InterfaceType::Gif;
  if (s == "tun")
    return InterfaceType::Tun;
  if (s == "gre")
    return InterfaceType::GRE;
  if (s == "vxlan")
    return InterfaceType::VXLAN;
  if (s == "ipsec")
    return InterfaceType::IPsec;
  if (s == "epair" || s == "virtual")
    return InterfaceType::Epair;
  if (s == "carp")
    return InterfaceType::Carp;
  if (s == "tap")
    return InterfaceType::Tap;
  if (s == "stf" || s == "6to4" || s == "sixtofour")
    return InterfaceType::SixToFour;
  if (s == "ovpn" || s == "openvpn")
    return InterfaceType::Ovpn;
  if (s == "pflog")
    return InterfaceType::Pflog;
  if (s == "pfsync")
    return InterfaceType::Pfsync;
  if (s == "wireguard" || s == "wg")
    return InterfaceType::WireGuard;
  if (s == "enc")
    return InterfaceType::Enc;
  if (s == "wireless" || s == "wlan")
    return InterfaceType::Wireless;
  return InterfaceType::Unknown;
}