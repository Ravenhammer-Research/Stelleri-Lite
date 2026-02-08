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
 * @file SystemConfigurationManager.hpp
 * @brief FreeBSD system-call-based configuration manager
 */

#pragma once

#include "ArpConfig.hpp"
#include "BridgeInterfaceConfig.hpp"
#include "ConfigurationManager.hpp"
#include "GREConfig.hpp"
#include "InterfaceConfig.hpp"
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
#include <optional>
#include <string_view>
#include <vector>

/**
 * @brief ConfigurationManager backed by FreeBSD system calls
 *
 * Implements the full ConfigurationManager interface using ioctl, sysctl,
 * routing sockets, and ifconfig-style operations on the local system.
 */
class SystemConfigurationManager : public ConfigurationManager {
public:
  // Instance overrides implement the abstract `ConfigurationManager` API.
  std::vector<InterfaceConfig> GetInterfaces(
      const std::optional<VRFConfig> &vrf = std::nullopt) const override;
  std::vector<InterfaceConfig>
  GetInterfacesByGroup(const std::optional<VRFConfig> &vrf,
                       std::string_view group) const override;
  std::vector<BridgeInterfaceConfig> GetBridgeInterfaces(
      const std::optional<VRFConfig> &vrf = std::nullopt) const override;
  std::vector<LaggConfig> GetLaggInterfaces(
      const std::optional<VRFConfig> &vrf = std::nullopt) const override;
  std::vector<VLANConfig> GetVLANInterfaces(
      const std::optional<VRFConfig> &vrf = std::nullopt) const override;
  std::vector<TunnelConfig> GetTunnelInterfaces(
      const std::optional<VRFConfig> &vrf = std::nullopt) const override;
  std::vector<VirtualInterfaceConfig> GetVirtualInterfaces(
      const std::optional<VRFConfig> &vrf = std::nullopt) const override;
  std::vector<GREConfig> GetGreInterfaces(
      const std::optional<VRFConfig> &vrf = std::nullopt) const override;
  std::vector<VXLANConfig> GetVxlanInterfaces(
      const std::optional<VRFConfig> &vrf = std::nullopt) const override;
  std::vector<RouteConfig> GetStaticRoutes(
      const std::optional<VRFConfig> &vrf = std::nullopt) const override;
  std::vector<RouteConfig>
  GetRoutes(const std::optional<VRFConfig> &vrf = std::nullopt) const override;
  std::vector<VRFConfig> GetNetworkInstances(
      const std::optional<int> &table = std::nullopt) const override;

  /** @brief Get the number of configured FIBs from net.fibs sysctl */
  int GetFibs() const override;

  // ARP/NDP neighbor cache management
  std::vector<ArpConfig>
  GetArpEntries(const std::optional<std::string> &ip_filter = std::nullopt,
                const std::optional<std::string> &iface_filter =
                    std::nullopt) const override;
  bool SetArpEntry(const std::string &ip, const std::string &mac,
                   const std::optional<std::string> &iface = std::nullopt,
                   bool temp = false, bool pub = false) const override;
  bool DeleteArpEntry(
      const std::string &ip,
      const std::optional<std::string> &iface = std::nullopt) const override;

  std::vector<NdpConfig>
  GetNdpEntries(const std::optional<std::string> &ip_filter = std::nullopt,
                const std::optional<std::string> &iface_filter =
                    std::nullopt) const override;
  bool SetNdpEntry(const std::string &ip, const std::string &mac,
                   const std::optional<std::string> &iface = std::nullopt,
                   bool temp = false) const override;
  bool DeleteNdpEntry(
      const std::string &ip,
      const std::optional<std::string> &iface = std::nullopt) const override;

public:
  enum class IfreqIntField { Metric, Fib, Mtu };

  // Helper methods for interface queries
  void prepare_ifreq(struct ifreq &ifr, const std::string &name) const;
  void cloneInterface(const std::string &name, unsigned long cmd) const;
  std::optional<int> query_ifreq_int(const std::string &ifname,
                                     unsigned long req,
                                     IfreqIntField which) const;
  std::optional<std::pair<std::string, int>>
  query_ifreq_sockaddr(const std::string &ifname, unsigned long req) const;
  std::vector<std::string>
  query_interface_groups(const std::string &ifname) const;
  void populateInterfaceMetadata(InterfaceConfig &ic) const;

  // Interface type detection
  bool interfaceIsLagg(const std::string &ifname) const;
  bool interfaceIsBridge(const std::string &ifname) const;

  // Return list of addresses configured on an interface (string format
  // "addr/prefix")
  std::vector<std::string> GetInterfaceAddresses(const std::string &ifname,
                                                 int family) const override;

  // Bridge-specific system operations
  void CreateBridge(const std::string &name) const override;
  void SaveBridge(const BridgeInterfaceConfig &bic) const override;
  std::vector<std::string>
  GetBridgeMembers(const std::string &name) const override;

  // Virtual interface (epair/clone) operations
  void CreateVirtual(const std::string &name) const override;
  void SaveVirtual(const VirtualInterfaceConfig &vic) const override;

  // LAGG-specific system operations
  void CreateLagg(const std::string &name) const override;
  void SaveLagg(const LaggConfig &lac) const override;

  // Generic interface operations
  void CreateInterface(const std::string &name) const override;
  void SaveInterface(const InterfaceConfig &ic) const override;
  void DestroyInterface(const std::string &name) const override;
  void RemoveInterfaceAddress(const std::string &ifname,
                              const std::string &addr) const override;
  void RemoveInterfaceGroup(const std::string &ifname,
                            const std::string &group) const override;
  // Query whether a named interface exists on the system
  bool InterfaceExists(std::string_view name) const override;

  // WLAN-specific operations
  void CreateWlan(const std::string &name) const override;
  void SaveWlan(const WlanConfig &wlan) const override;

  // TAP-specific operations
  void CreateTap(const std::string &name) const override;
  void SaveTap(const TapConfig &tap) const override;

  // GRE-specific operations
  void CreateGre(const std::string &name) const override;
  void SaveGre(const GREConfig &gre) const override;

  // VXLAN-specific operations
  void CreateVxlan(const std::string &name) const override;
  void SaveVxlan(const VXLANConfig &vxlan) const override;

  // CARP-specific operations
  void SaveCarp(const CarpConfig &carp) const override;

  // Tunnel-specific operations
  void CreateTunnel(const std::string &name) const override;
  void SaveTunnel(const TunnelConfig &tunnel) const override;

  // VLAN-specific operations
  void SaveVlan(const VLANConfig &vlan) const override;

  // Route operations
  void AddRoute(const RouteConfig &route) const override;
  void DeleteRoute(const RouteConfig &route) const override;

  // VRF matching helper
  bool matches_vrf(const InterfaceConfig &ic,
                   const std::optional<VRFConfig> &vrf) const;
};
