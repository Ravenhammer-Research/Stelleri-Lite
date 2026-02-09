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
#include <string_view>
#include <vector>
// Ensure configuration type declarations are available for method signatures
#include "ArpConfig.hpp"
#include "BridgeInterfaceConfig.hpp"
#include "CarpConfig.hpp"
#include "GREConfig.hpp"
#include "LaggConfig.hpp"
#include "NdpConfig.hpp"
#include "RouteConfig.hpp"
#include "TapConfig.hpp"
#include "TunnelConfig.hpp"
#include "VLANConfig.hpp"
#include "VRFConfig.hpp"
#include "VXLANConfig.hpp"
#include "VirtualInterfaceConfig.hpp"
#include "WlanConfig.hpp"

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

  // ── Enumeration / query API ──────────────────────────────────────────

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
  virtual std::vector<GREConfig> GetGreInterfaces(
      const std::optional<VRFConfig> &vrf = std::nullopt) const = 0;
  virtual std::vector<VXLANConfig> GetVxlanInterfaces(
      const std::optional<VRFConfig> &vrf = std::nullopt) const = 0;
  virtual std::vector<RouteConfig>
  GetStaticRoutes(const std::optional<VRFConfig> &vrf = std::nullopt) const = 0;
  virtual std::vector<RouteConfig>
  GetRoutes(const std::optional<VRFConfig> &vrf = std::nullopt) const = 0;
    virtual std::vector<VRFConfig> GetVrfs() const = 0;

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

  // ── Mutation API ─────────────────────────────────────────────────────

  // Generic interface operations
  virtual void CreateInterface(const std::string &name) const = 0;
  virtual void SaveInterface(const InterfaceConfig &ic) const = 0;
  virtual void DestroyInterface(const std::string &name) const = 0;
  virtual void RemoveInterfaceAddress(const std::string &ifname,
                                      const std::string &addr) const = 0;
  virtual void RemoveInterfaceGroup(const std::string &ifname,
                                    const std::string &group) const = 0;
  virtual bool InterfaceExists(std::string_view name) const = 0;
  virtual std::vector<std::string>
  GetInterfaceAddresses(const std::string &ifname, int family) const = 0;

  // Bridge operations
  virtual void CreateBridge(const std::string &name) const = 0;
  virtual void SaveBridge(const BridgeInterfaceConfig &bic) const = 0;
  virtual std::vector<std::string>
  GetBridgeMembers(const std::string &name) const = 0;

  // LAGG operations
  virtual void CreateLagg(const std::string &name) const = 0;
  virtual void SaveLagg(const LaggConfig &lac) const = 0;

  // VLAN operations
  virtual void SaveVlan(const VLANConfig &vlan) const = 0;

  // Tunnel operations
  virtual void CreateTunnel(const std::string &name) const = 0;
  virtual void SaveTunnel(const TunnelConfig &tunnel) const = 0;

  // Virtual interface (epair/clone) operations
  virtual void CreateVirtual(const std::string &name) const = 0;
  virtual void SaveVirtual(const VirtualInterfaceConfig &vic) const = 0;

  // WLAN operations
  virtual void CreateWlan(const std::string &name) const = 0;
  virtual void SaveWlan(const WlanConfig &wlan) const = 0;

  // TAP operations
  virtual void CreateTap(const std::string &name) const = 0;
  virtual void SaveTap(const TapConfig &tap) const = 0;

  // GRE operations
  virtual void CreateGre(const std::string &name) const = 0;
  virtual void SaveGre(const GREConfig &gre) const = 0;

  // VXLAN operations
  virtual void CreateVxlan(const std::string &name) const = 0;
  virtual void SaveVxlan(const VXLANConfig &vxlan) const = 0;

  // CARP operations
  virtual void SaveCarp(const CarpConfig &carp) const = 0;

  // Route operations
  virtual void AddRoute(const RouteConfig &route) const = 0;
  virtual void DeleteRoute(const RouteConfig &route) const = 0;

    // VRF helpers
    // Use `GetVrfs(...)` for retrieving VRF definitions.

  // ── Convenience helpers ──────────────────────────────────────────────

  /// Look up a single interface by name.
  std::optional<InterfaceConfig> GetInterface(const std::string &name) const {
    auto ifs = GetInterfaces();
    for (auto &i : ifs) {
      if (i.name == name)
        return std::optional<InterfaceConfig>(std::move(i));
    }
    return std::nullopt;
  }
};
