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
 * @brief NETCONF-based configuration manager (stub)
 *
 * Placeholder implementation of ConfigurationManager that will communicate
 * with a remote device via NETCONF/YANG rather than local system calls.
 */

#pragma once

#include "ConfigurationManager.hpp"
#include "WlanInterfaceConfig.hpp"
#include <stdexcept>

class NetconfConfigurationManager : public ConfigurationManager {
public:
  // ── Enumeration / query API ──────────────────────────────────────────

  std::vector<InterfaceConfig> GetInterfaces(
      const std::optional<VRFConfig> & /*vrf*/ = std::nullopt) const override {
    return {};
  }

  std::vector<InterfaceConfig>
  GetInterfacesByGroup(const std::optional<VRFConfig> & /*vrf*/,
                       std::string_view /*group*/) const override {
    return {};
  }

  std::vector<BridgeInterfaceConfig> GetBridgeInterfaces(
      const std::optional<VRFConfig> & /*vrf*/ = std::nullopt) const override {
    return {};
  }

  std::vector<LaggConfig> GetLaggInterfaces(
      const std::optional<VRFConfig> & /*vrf*/ = std::nullopt) const override {
    return {};
  }

  std::vector<VLANConfig> GetVLANInterfaces(
      const std::optional<VRFConfig> & /*vrf*/ = std::nullopt) const override {
    return {};
  }

  std::vector<TunConfig> GetTunInterfaces(
      const std::optional<VRFConfig> & /*vrf*/ = std::nullopt) const override {
    return {};
  }
  std::vector<GifConfig> GetGifInterfaces(
      const std::optional<VRFConfig> & /*vrf*/ = std::nullopt) const override {
    return {};
  }
  std::vector<OvpnConfig> GetOvpnInterfaces(
      const std::optional<VRFConfig> & /*vrf*/ = std::nullopt) const override {
    return {};
  }
  std::vector<IpsecConfig> GetIpsecInterfaces(
      const std::optional<VRFConfig> & /*vrf*/ = std::nullopt) const override {
    return {};
  }

  std::vector<VirtualInterfaceConfig> GetVirtualInterfaces(
      const std::optional<VRFConfig> & /*vrf*/ = std::nullopt) const override {
    return {};
  }

  std::vector<WlanInterfaceConfig> GetWlanInterfaces(
      const std::optional<VRFConfig> & /*vrf*/ = std::nullopt) const override {
    return {};
  }

  std::vector<RouteConfig> GetStaticRoutes(
      const std::optional<VRFConfig> & /*vrf*/ = std::nullopt) const override {
    return {};
  }

  std::vector<RouteConfig> GetRoutes(
      const std::optional<VRFConfig> & /*vrf*/ = std::nullopt) const override {
    return {};
  }

  std::vector<VRFConfig> GetVrfs() const override { return {}; }

  // ARP/NDP
  std::vector<ArpConfig>
  GetArpEntries(const std::optional<std::string> & /*ip*/ = std::nullopt,
                const std::optional<std::string> & /*iface*/ =
                    std::nullopt) const override {
    return {};
  }

  bool SetArpEntry(const std::string & /*ip*/, const std::string & /*mac*/,
                   const std::optional<std::string> & /*iface*/ = std::nullopt,
                   bool /*temp*/ = false, bool /*pub*/ = false) const override {
    return false;
  }

  bool DeleteArpEntry(const std::string & /*ip*/,
                      const std::optional<std::string> & /*iface*/ =
                          std::nullopt) const override {
    return false;
  }

  std::vector<NdpConfig>
  GetNdpEntries(const std::optional<std::string> & /*ip*/ = std::nullopt,
                const std::optional<std::string> & /*iface*/ =
                    std::nullopt) const override {
    return {};
  }

  bool SetNdpEntry(const std::string & /*ip*/, const std::string & /*mac*/,
                   const std::optional<std::string> & /*iface*/ = std::nullopt,
                   bool /*temp*/ = false) const override {
    return false;
  }

  bool DeleteNdpEntry(const std::string & /*ip*/,
                      const std::optional<std::string> & /*iface*/ =
                          std::nullopt) const override {
    return false;
  }

  // ── Mutation API ─────────────────────────────────────────────────────

  void CreateInterface(const std::string & /*name*/) const override {}
  void SaveInterface(const InterfaceConfig & /*ic*/) const override {}
  void DestroyInterface(const std::string & /*name*/) const override {}
  void RemoveInterfaceAddress(const std::string & /*ifname*/,
                              const std::string & /*addr*/) const override {}
  void RemoveInterfaceGroup(const std::string & /*ifname*/,
                            const std::string & /*group*/) const override {}
  bool InterfaceExists(std::string_view /*name*/) const override {
    return false;
  }
  std::vector<std::string>
  GetInterfaceAddresses(const std::string & /*ifname*/,
                        int /*family*/) const override {
    return {};
  }

  void CreateBridge(const std::string & /*name*/) const override {}
  void SaveBridge(const BridgeInterfaceConfig & /*bic*/) const override {}
  std::vector<std::string>
  GetBridgeMembers(const std::string & /*name*/) const override {
    return {};
  }

  void CreateLagg(const std::string & /*name*/) const override {}
  void SaveLagg(const LaggConfig & /*lac*/) const override {}

  void SaveVlan(const VLANConfig & /*vlan*/) const override {}

  void CreateTun(const std::string & /*name*/) const override {}
  void SaveTun(const TunConfig & /*tun*/) const override {}
  void CreateGif(const std::string & /*name*/) const override {}
  void SaveGif(const GifConfig & /*gif*/) const override {}
  void CreateOvpn(const std::string & /*name*/) const override {}
  void SaveOvpn(const OvpnConfig & /*ovpn*/) const override {}
  void CreateIpsec(const std::string & /*name*/) const override {}
  void SaveIpsec(const IpsecConfig & /*ipsec*/) const override {}

  void CreateVirtual(const std::string & /*name*/) const override {}
  void SaveVirtual(const VirtualInterfaceConfig & /*vic*/) const override {}

  void CreateWlan(const std::string & /*name*/) const override {}
  void SaveWlan(const WlanConfig & /*wlan*/) const override {}

  void CreateTap(const std::string & /*name*/) const override {}
  void SaveTap(const TapConfig & /*tap*/) const override {}

  void AddRoute(const RouteConfig & /*route*/) const override {}
  void DeleteRoute(const RouteConfig & /*route*/) const override {}

  int GetFibs() const override { return 1; }
};
