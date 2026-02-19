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
 * @file VXLANConfig.hpp
 * @brief VXLAN tunnel interface configuration
 */

#pragma once

#include "ConfigurationManager.hpp"
#include "InterfaceConfig.hpp"
#include <optional>
#include <string>

/**
 * @brief Configuration for VXLAN overlay interfaces
 *
 * Wraps the FreeBSD if_vxlan(4) parameters from struct ifvxlancfg:
 * VNI, local/remote addresses, ports, TTL, and learning behaviour.
 */
class VxlanInterfaceConfig : public InterfaceConfig {
public:
  explicit VxlanInterfaceConfig(const InterfaceConfig &base)
      : InterfaceConfig(base) {}

  /// VXLAN Network Identifier (24-bit)
  std::optional<uint32_t> vni;

  /// Local VTEP address
  std::optional<std::string> localAddr;

  /// Remote VTEP / multicast group address
  std::optional<std::string> remoteAddr;

  /// Local UDP port (default 4789)
  std::optional<uint16_t> localPort;

  /// Remote UDP port
  std::optional<uint16_t> remotePort;

  /// IP TTL for encapsulated packets
  std::optional<uint8_t> ttl;

  /// MAC learning enabled
  std::optional<bool> learn;

  /// Multicast interface name for BUM traffic
  std::optional<std::string> multicastIf;

  /// Minimum source UDP port for port range
  std::optional<uint16_t> portMin;

  /// Maximum source UDP port for port range
  std::optional<uint16_t> portMax;

  /// Forwarding table entry timeout in seconds
  std::optional<uint32_t> ftableTimeout;

  /// Maximum forwarding table entries
  std::optional<uint32_t> ftableMax;
  void save(ConfigurationManager &mgr) const override;
  void create(ConfigurationManager &mgr) const;
  void destroy(ConfigurationManager &mgr) const override;
};
