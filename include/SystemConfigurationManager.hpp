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

#pragma once

#include "ArpConfig.hpp"
#include "BridgeInterfaceConfig.hpp"
#include "ConfigurationManager.hpp"
#include "InterfaceConfig.hpp"
#include "LaggConfig.hpp"
#include "NdpConfig.hpp"
#include "RouteConfig.hpp"
#include "TunnelConfig.hpp"
#include "VLANConfig.hpp"
#include "VRFConfig.hpp"
#include "VirtualInterfaceConfig.hpp"
#include <optional>
#include <string_view>
#include <vector>

/**
 * @brief System configuration helpers (enumeration only)
 *
 * This simplified declaration provides only the static enumeration helpers
 * requested: methods that return smart-pointer-managed arrays of interface
 * configuration objects for each specific interface type.
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
  std::vector<RouteConfig> GetStaticRoutes(
      const std::optional<VRFConfig> &vrf = std::nullopt) const override;
  std::vector<RouteConfig>
  GetRoutes(const std::optional<VRFConfig> &vrf = std::nullopt) const override;
  std::vector<VRFConfig> GetNetworkInstances(
      const std::optional<int> &table = std::nullopt) const override;

  /** @brief Get the number of configured FIBs from net.fibs sysctl */
  int GetFibs() const;

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

private:
  enum class IfreqIntField { Metric, Fib, Mtu };

  // Helper methods for interface queries
  void prepare_ifreq(struct ifreq &ifr, const std::string &name) const;
  std::optional<int> query_ifreq_int(const std::string &ifname,
                                     unsigned long req,
                                     IfreqIntField which) const;
  std::optional<std::pair<std::string, int>>
  query_ifreq_sockaddr(const std::string &ifname, unsigned long req) const;
  std::vector<std::string>
  query_interface_groups(const std::string &ifname) const;
  std::optional<std::string>
  query_ifstatus_nd6(const std::string &ifname) const;
  void populateInterfaceMetadata(InterfaceConfig &ic) const;

  // Interface type detection
  bool interfaceIsLagg(const std::string &ifname) const;
  bool interfaceIsBridge(const std::string &ifname) const;

  // VRF matching helper
  bool matches_vrf(const InterfaceConfig &ic,
                   const std::optional<VRFConfig> &vrf) const;
};
