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
 * @file ConfigurationManager.hpp
 * @brief Abstract configuration management interface
 */

#pragma once

#include "ConfigData.hpp"
#include "InterfaceConfig.hpp"
#include <memory>
#include <optional>
#include <string>
#include <vector>
// Ensure configuration type declarations are available for method signatures
#include "ArpConfig.hpp"
#include "BridgeInterfaceConfig.hpp"
#include "LaggConfig.hpp"
#include "NdpConfig.hpp"
#include "RouteConfig.hpp"
#include "TunnelConfig.hpp"
#include "VLANConfig.hpp"
#include "VRFConfig.hpp"
#include "VirtualInterfaceConfig.hpp"

/**
 * @brief Abstract base class for configuration storage and retrieval
 *
 * Provides an interface for setting and querying network configuration data.
 * Implementations can use in-memory storage, file-based persistence, or other
 * backends.
 */
class ConfigurationManager {
public:
  virtual ~ConfigurationManager() = default;

  // Enumeration API implemented by system-backed managers
  virtual std::vector<InterfaceConfig>
  GetInterfaces(const std::optional<VRFConfig> &vrf = std::nullopt) const = 0;
  virtual std::vector<InterfaceConfig>
  GetInterfacesByGroup(const std::optional<VRFConfig> &vrf,
                       std::string_view group) const = 0;
  virtual std::vector<BridgeInterfaceConfig> GetBridgeInterfaces(
      const std::optional<VRFConfig> &vrf = std::nullopt) const = 0;
  virtual std::vector<LaggConfig> GetLaggInterfaces(
      const std::optional<VRFConfig> &vrf = std::nullopt) const = 0;
  virtual std::vector<VLANConfig> GetVLANInterfaces(
      const std::optional<VRFConfig> &vrf = std::nullopt) const = 0;
  virtual std::vector<TunnelConfig> GetTunnelInterfaces(
      const std::optional<VRFConfig> &vrf = std::nullopt) const = 0;
  virtual std::vector<VirtualInterfaceConfig> GetVirtualInterfaces(
      const std::optional<VRFConfig> &vrf = std::nullopt) const = 0;
  virtual std::vector<RouteConfig>
  GetStaticRoutes(const std::optional<VRFConfig> &vrf = std::nullopt) const = 0;
  virtual std::vector<RouteConfig>
  GetRoutes(const std::optional<VRFConfig> &vrf = std::nullopt) const = 0;
  virtual std::vector<VRFConfig>
  GetNetworkInstances(const std::optional<int> &table = std::nullopt) const = 0;

  // ARP/NDP neighbor cache management
  virtual std::vector<ArpConfig> GetArpEntries(
      const std::optional<std::string> &ip_filter = std::nullopt,
      const std::optional<std::string> &iface_filter = std::nullopt) const = 0;
  virtual bool
  SetArpEntry(const std::string &ip, const std::string &mac,
              const std::optional<std::string> &iface = std::nullopt,
              bool temp = false, bool pub = false) const = 0;
  virtual bool DeleteArpEntry(
      const std::string &ip,
      const std::optional<std::string> &iface = std::nullopt) const = 0;

  virtual std::vector<NdpConfig> GetNdpEntries(
      const std::optional<std::string> &ip_filter = std::nullopt,
      const std::optional<std::string> &iface_filter = std::nullopt) const = 0;
  virtual bool
  SetNdpEntry(const std::string &ip, const std::string &mac,
              const std::optional<std::string> &iface = std::nullopt,
              bool temp = false) const = 0;
  virtual bool DeleteNdpEntry(
      const std::string &ip,
      const std::optional<std::string> &iface = std::nullopt) const = 0;

  // Backwards-compatible helper: return interfaces as sliced `ConfigData`
  std::vector<InterfaceConfig> getInterfaces() const { return GetInterfaces(); }

  // Backwards-compatible helper: get single interface by name
  std::optional<InterfaceConfig> getInterface(const std::string &name) const {
    auto ifs = GetInterfaces();
    for (auto &i : ifs) {
      if (i.name == name) {
        return std::optional<InterfaceConfig>(std::move(i));
      }
    }
    return std::nullopt;
  }

  // Backwards-compatible helper: get routes as sliced ConfigData
  std::vector<RouteConfig> getRoutes() const { return GetRoutes(); }
};
