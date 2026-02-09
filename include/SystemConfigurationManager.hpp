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
 * @brief Thin system-specific ConfigurationManager helpers
 */

#pragma once

#include "ConfigurationManager.hpp"
#include "InterfaceConfig.hpp"
#include "VRFConfig.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <utility>

struct ifreq;

class SystemConfigurationManager : public ConfigurationManager {
public:
  ~SystemConfigurationManager() override = default;

  // Enumeration / query API
  std::vector<InterfaceConfig>
  GetInterfaces(const std::optional<VRFConfig> &vrf = std::nullopt) const override;
  std::vector<InterfaceConfig>
  GetInterfacesByGroup(const std::optional<VRFConfig> &vrf,
					   std::string_view group) const override;
  std::vector<BridgeInterfaceConfig> GetBridgeInterfaces(
	  const std::optional<VRFConfig> &vrf = std::nullopt) const override;
  std::vector<LaggInterfaceConfig> GetLaggInterfaces(
	  const std::optional<VRFConfig> &vrf = std::nullopt) const override;
  std::vector<VlanInterfaceConfig> GetVLANInterfaces(
	  const std::optional<VRFConfig> &vrf = std::nullopt) const override;

  std::vector<TunInterfaceConfig> GetTunInterfaces(
	  const std::optional<VRFConfig> &vrf = std::nullopt) const override;
  std::vector<GifInterfaceConfig> GetGifInterfaces(
	  const std::optional<VRFConfig> &vrf = std::nullopt) const override;
  std::vector<OvpnInterfaceConfig> GetOvpnInterfaces(
	  const std::optional<VRFConfig> &vrf = std::nullopt) const override;
  std::vector<IpsecInterfaceConfig> GetIpsecInterfaces(
	  const std::optional<VRFConfig> &vrf = std::nullopt) const override;
  std::vector<GreInterfaceConfig> GetGreInterfaces(
	  const std::optional<VRFConfig> &vrf = std::nullopt) const override;
  std::vector<VxlanInterfaceConfig> GetVxlanInterfaces(
	  const std::optional<VRFConfig> &vrf = std::nullopt) const override;
  std::vector<EpairInterfaceConfig> GetEpairInterfaces(
	  const std::optional<VRFConfig> &vrf = std::nullopt) const override;
  std::vector<RouteConfig>
  GetStaticRoutes(const std::optional<VRFConfig> &vrf = std::nullopt) const override;
  std::vector<RouteConfig>
  GetRoutes(const std::optional<VRFConfig> &vrf = std::nullopt) const override;
  std::vector<VRFConfig> GetVrfs() const override;

  std::vector<ArpConfig> GetArpEntries(
	  const std::optional<std::string> &ip_filter = std::nullopt,
	  const std::optional<std::string> &iface_filter = std::nullopt) const override;
  bool SetArpEntry(const std::string &ip, const std::string &mac,
				   const std::optional<std::string> &iface = std::nullopt,
				   bool temp = false, bool pub = false) const override;
  bool DeleteArpEntry(const std::string &ip,
					  const std::optional<std::string> &iface = std::nullopt) const override;

  std::vector<NdpConfig> GetNdpEntries(
	  const std::optional<std::string> &ip_filter = std::nullopt,
	  const std::optional<std::string> &iface_filter = std::nullopt) const override;
  bool SetNdpEntry(const std::string &ip, const std::string &mac,
				   const std::optional<std::string> &iface = std::nullopt,
				   bool temp = false) const override;
  bool DeleteNdpEntry(const std::string &ip,
					  const std::optional<std::string> &iface = std::nullopt) const override;

  // Mutation API
  void CreateInterface(const std::string &name) const override;
  void SaveInterface(const InterfaceConfig &ic) const override;
  void DestroyInterface(const std::string &name) const override;
  void RemoveInterfaceAddress(const std::string &ifname,
							  const std::string &addr) const override;
  void RemoveInterfaceGroup(const std::string &ifname,
							const std::string &group) const override;
  bool InterfaceExists(std::string_view name) const override;
  std::vector<std::string>
  GetInterfaceAddresses(const std::string &ifname, int family) const override;

  // Helper methods (overrides of ConfigurationManager convenience helpers)
  void prepare_ifreq(struct ifreq &ifr, const std::string &name) const override;
  void cloneInterface(const std::string &name, unsigned long cmd) const override;
  std::optional<int> query_ifreq_int(const std::string &ifname,
                                     unsigned long req,
                                     IfreqIntField which) const override;
  std::optional<std::pair<std::string, int>>
  query_ifreq_sockaddr(const std::string &ifname, unsigned long req) const override;
  std::vector<std::string> query_interface_groups(const std::string &ifname) const override;
  void populateInterfaceMetadata(InterfaceConfig &ic) const override;
  // VRF matching helper
  bool matches_vrf(const InterfaceConfig &ic, const std::optional<VRFConfig> &vrf) const override;

  // Bridge
  void CreateBridge(const std::string &name) const override;
  void SaveBridge(const BridgeInterfaceConfig &bic) const override;
  std::vector<std::string> GetBridgeMembers(const std::string &name) const override;

  // LAGG
  void CreateLagg(const std::string &name) const override;
  void SaveLagg(const LaggInterfaceConfig &lac) const override;

  // VLAN
  void SaveVlan(const VlanInterfaceConfig &vlan) const override;

  // Tunnel types
  void CreateTun(const std::string &name) const override;
  void SaveTun(const TunInterfaceConfig &tun) const override;
  void CreateGif(const std::string &name) const override;
  void SaveGif(const GifInterfaceConfig &gif) const override;
  void CreateOvpn(const std::string &name) const override;
  void SaveOvpn(const OvpnInterfaceConfig &ovpn) const override;
  void CreateIpsec(const std::string &name) const override;
  void SaveIpsec(const IpsecInterfaceConfig &ipsec) const override;

  // WLAN
  void CreateWlan(const std::string &name) const override;
  void SaveWlan(const WlanInterfaceConfig &wlan) const override;

  // TAP
  void CreateTap(const std::string &name) const override;
  void SaveTap(const TapInterfaceConfig &tap) const override;

  // GRE
  void CreateGre(const std::string &name) const override;
  void SaveGre(const GreInterfaceConfig &gre) const override;

  // VXLAN
  void CreateVxlan(const std::string &name) const override;
  void SaveVxlan(const VxlanInterfaceConfig &vxlan) const override;

  // CARP
  void SaveCarp(const CarpInterfaceConfig &carp) const override;

  // Routes
  void AddRoute(const RouteConfig &route) const override;
  void DeleteRoute(const RouteConfig &route) const override;

  // Epair
  void CreateEpair(const std::string &name) const override;
  void SaveEpair(const EpairInterfaceConfig &epair) const override;
  bool interfaceIsLagg(const std::string &ifname) const override;
  bool interfaceIsBridge(const std::string &ifname) const override;
};
