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
 * @file NetconfConfigurationManager.hpp
 * @brief NETCONF-based configuration manager
 *
 */

#pragma once

#if !defined(STELLERI_NETCONF) || STELLERI_NETCONF != 1
#error "netconf headers are for the STELLERI_NETCONF build only"
#endif

#include "BridgeInterfaceConfig.hpp"
#include "CarpInterfaceConfig.hpp"
#include "ConfigurationManager.hpp"
#include "EpairInterfaceConfig.hpp"
#include "GifInterfaceConfig.hpp"
#include "GreInterfaceConfig.hpp"
#include "IpsecInterfaceConfig.hpp"
#include "LaggInterfaceConfig.hpp"
#include "OvpnInterfaceConfig.hpp"
#include "PolicyConfig.hpp"
#include "TunInterfaceConfig.hpp"
#include "VlanInterfaceConfig.hpp"
#include "VxlanInterfaceConfig.hpp"
#include "WlanInterfaceConfig.hpp"
#include <stdexcept>

// Forward-declare system struct used by the ConfigurationManager helper API so
// netconf backend headers don't need to include <net/if.h>.
struct ifreq;

#include <optional>
#include <string>
#include <vector>

class NetconfConfigurationManager : public ConfigurationManager {
  // Enumeration / query API
  std::vector<InterfaceConfig> GetInterfaces(
      const std::optional<VRFConfig> &vrf = std::nullopt) const override;
  std::vector<InterfaceConfig>
  GetInterfacesByGroup(const std::optional<VRFConfig> &vrf,
                       std::string_view group) const override;
  std::vector<BridgeInterfaceConfig>
  GetBridgeInterfaces(const std::vector<InterfaceConfig> &bases) const override;
  std::vector<LaggInterfaceConfig>
  GetLaggInterfaces(const std::vector<InterfaceConfig> &bases) const override;
  std::vector<VlanInterfaceConfig>
  GetVLANInterfaces(const std::vector<InterfaceConfig> &bases) const override;

  std::vector<TunInterfaceConfig>
  GetTunInterfaces(const std::vector<InterfaceConfig> &bases) const override;
  std::vector<GifInterfaceConfig>
  GetGifInterfaces(const std::vector<InterfaceConfig> &bases) const override;
  std::vector<OvpnInterfaceConfig>
  GetOvpnInterfaces(const std::vector<InterfaceConfig> &bases) const override;
  std::vector<IpsecInterfaceConfig>
  GetIpsecInterfaces(const std::vector<InterfaceConfig> &bases) const override;
  std::vector<GreInterfaceConfig>
  GetGreInterfaces(const std::vector<InterfaceConfig> &bases) const override;
  std::vector<VxlanInterfaceConfig>
  GetVxlanInterfaces(const std::vector<InterfaceConfig> &bases) const override;
  std::vector<EpairInterfaceConfig>
  GetEpairInterfaces(const std::vector<InterfaceConfig> &bases) const override;
  std::vector<WlanInterfaceConfig>
  GetWlanInterfaces(const std::vector<InterfaceConfig> &bases) const override;
  std::vector<CarpInterfaceConfig>
  GetCarpInterfaces(const std::vector<InterfaceConfig> &bases) const override;
  std::vector<RouteConfig> GetStaticRoutes(
      const std::optional<VRFConfig> &vrf = std::nullopt) const override;
  std::vector<RouteConfig>
  GetRoutes(const std::optional<VRFConfig> &vrf = std::nullopt) const override;
  std::vector<VRFConfig> GetVrfs() const override;

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

  // Mutation API
  void CreateInterface(const std::string &name) const override;
  void SaveInterface(const InterfaceConfig &ic) const override;
  void DestroyInterface(const std::string &name) const override;
  void RemoveInterfaceAddress(const std::string &ifname,
                              const std::string &addr) const override;
  void RemoveInterfaceGroup(const std::string &ifname,
                            const std::string &group) const override;
  bool InterfaceExists(std::string_view name) const override;
  std::vector<std::string> GetInterfaceAddresses(const std::string &ifname,
                                                 int family) const override;

  // Bridge
  void CreateBridge(const std::string &name) const override;
  void SaveBridge(const BridgeInterfaceConfig &bic) const override;
  std::vector<std::string>
  GetBridgeMembers(const std::string &name) const override;

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
  void CreateSixToFour(const std::string &name) const override;
  void SaveSixToFour(const SixToFourInterfaceConfig &t) const override;
  void DestroySixToFour(const std::string &name) const override;
  std::vector<SixToFourInterfaceConfig> GetSixToFourInterfaces(
      const std::vector<InterfaceConfig> &bases) const override;

  // WLAN
  void CreateWlan(const std::string &name) const override;
  void SaveWlan(const WlanInterfaceConfig &wlan) const override;

  // TAP
  void CreateTap(const std::string &name) const override;
  void SaveTap(const TapInterfaceConfig &tap) const override;

  // PFLOG
  void CreatePflog(const std::string &name) const override;
  void SavePflog(const PflogInterfaceConfig &p) const override;
  void DestroyPflog(const std::string &name) const override;
  std::vector<PflogInterfaceConfig>
  GetPflogInterfaces(const std::vector<InterfaceConfig> &bases) const override;

  // GRE
  void CreateGre(const std::string &name) const override;
  void SaveGre(const GreInterfaceConfig &gre) const override;

  // VXLAN
  void CreateVxlan(const std::string &name) const override;
  void SaveVxlan(const VxlanInterfaceConfig &vxlan) const override;

  // PFSYNC
  void CreatePfsync(const std::string &name) const override;
  void SavePfsync(const PfsyncInterfaceConfig &p) const override;
  void DestroyPfsync(const std::string &name) const override;
  std::vector<PfsyncInterfaceConfig>
  GetPfsyncInterfaces(const std::vector<InterfaceConfig> &bases) const override;

  // CARP
  void SaveCarp(const CarpInterfaceConfig &carp) const override;

  // Routes
  void AddRoute(const RouteConfig &route) const override;
  void DeleteRoute(const RouteConfig &route) const override;

  // Epair
  void CreateEpair(const std::string &name) const override;
  void SaveEpair(const EpairInterfaceConfig &epair) const override;

  // Policy
  std::vector<PolicyConfig> GetPolicies(
      const std::optional<uint32_t> &acl_filter = std::nullopt) const override;
  void SetPolicy(const PolicyConfig &pc) const override;
  void DeletePolicy(const PolicyConfig &pc) const override;

  // VRF
  void CreateVrf(const VRFConfig &vrf) const override;
  void DeleteVrf(const std::string &name) const override;
};
