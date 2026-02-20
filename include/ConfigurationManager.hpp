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

class ArpConfig;
class BridgeInterfaceConfig;
class GreInterfaceConfig;
class InterfaceConfig;
class LaggInterfaceConfig;
class NdpConfig;
class PolicyConfig;
class RouteConfig;
class TunInterfaceConfig;
class GifInterfaceConfig;
class OvpnInterfaceConfig;
class IpsecInterfaceConfig;
class VlanInterfaceConfig;
class VRFConfig;
class VxlanInterfaceConfig;
class EpairInterfaceConfig;
class WlanInterfaceConfig;
class TapInterfaceConfig;
class CarpInterfaceConfig;
class PflogInterfaceConfig;
class PfsyncInterfaceConfig;
class SixToFourInterfaceConfig;

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
  virtual std::vector<BridgeInterfaceConfig>
  GetBridgeInterfaces(const std::vector<InterfaceConfig> &bases) const = 0;
  virtual std::vector<LaggInterfaceConfig>
  GetLaggInterfaces(const std::vector<InterfaceConfig> &bases) const = 0;
  virtual std::vector<VlanInterfaceConfig>
  GetVLANInterfaces(const std::vector<InterfaceConfig> &bases) const = 0;

  virtual std::vector<TunInterfaceConfig>
  GetTunInterfaces(const std::vector<InterfaceConfig> &bases) const = 0;
  virtual std::vector<GifInterfaceConfig>
  GetGifInterfaces(const std::vector<InterfaceConfig> &bases) const = 0;
  virtual std::vector<OvpnInterfaceConfig>
  GetOvpnInterfaces(const std::vector<InterfaceConfig> &bases) const = 0;
  virtual std::vector<IpsecInterfaceConfig>
  GetIpsecInterfaces(const std::vector<InterfaceConfig> &bases) const = 0;
  virtual std::vector<GreInterfaceConfig>
  GetGreInterfaces(const std::vector<InterfaceConfig> &bases) const = 0;
  virtual std::vector<VxlanInterfaceConfig>
  GetVxlanInterfaces(const std::vector<InterfaceConfig> &bases) const = 0;
  virtual std::vector<EpairInterfaceConfig>
  GetEpairInterfaces(const std::vector<InterfaceConfig> &bases) const = 0;
  virtual std::vector<WlanInterfaceConfig>
  GetWlanInterfaces(const std::vector<InterfaceConfig> &bases) const = 0;
  virtual std::vector<CarpInterfaceConfig>
  GetCarpInterfaces(const std::vector<InterfaceConfig> &bases) const = 0;
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
  virtual void SaveLagg(const LaggInterfaceConfig &lac) const = 0;

  // VLAN operations
  virtual void SaveVlan(const VlanInterfaceConfig &vlan) const = 0;

  // Tunnel operations (specific types below)

  virtual void CreateTun(const std::string &name) const = 0;
  virtual void SaveTun(const TunInterfaceConfig &tun) const = 0;
  virtual void CreateGif(const std::string &name) const = 0;
  virtual void SaveGif(const GifInterfaceConfig &gif) const = 0;
  virtual void CreateOvpn(const std::string &name) const = 0;
  virtual void SaveOvpn(const OvpnInterfaceConfig &ovpn) const = 0;
  virtual void CreateIpsec(const std::string &name) const = 0;
  virtual void SaveIpsec(const IpsecInterfaceConfig &ipsec) const = 0;

  // WLAN operations
  virtual void CreateWlan(const std::string &name) const = 0;
  virtual void SaveWlan(const WlanInterfaceConfig &wlan) const = 0;

  // TAP operations
  virtual void CreateTap(const std::string &name) const = 0;
  virtual void SaveTap(const TapInterfaceConfig &tap) const = 0;

  // GRE operations
  virtual void CreateGre(const std::string &name) const = 0;
  virtual void SaveGre(const GreInterfaceConfig &gre) const = 0;

  // VXLAN operations
  virtual void CreateVxlan(const std::string &name) const = 0;
  virtual void SaveVxlan(const VxlanInterfaceConfig &vxlan) const = 0;

  // CARP operations
  virtual void SaveCarp(const CarpInterfaceConfig &carp) const = 0;

  // Route operations
  virtual void AddRoute(const RouteConfig &route) const = 0;
  virtual void DeleteRoute(const RouteConfig &route) const = 0;

  // Policy operations (access-lists, prefix-lists, route-maps)
  virtual std::vector<PolicyConfig> GetPolicies(
      const std::optional<uint32_t> &acl_filter = std::nullopt) const = 0;
  virtual void SetPolicy(const PolicyConfig &pc) const = 0;
  virtual void DeletePolicy(const PolicyConfig &pc) const = 0;

  virtual void CreateEpair(const std::string &name) const = 0;
  virtual void SaveEpair(const EpairInterfaceConfig &epair) const = 0;

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
  enum class IfreqIntField { Metric, Fib, Mtu };

  // Helper methods used by system-specific implementations (defined in
  // src/system/freebsd/*.cpp)
  virtual void prepare_ifreq(struct ifreq &ifr,
                             const std::string &name) const = 0;
  virtual void cloneInterface(const std::string &name,
                              unsigned long cmd) const = 0;
  virtual std::optional<int> query_ifreq_int(const std::string &ifname,
                                             unsigned long req,
                                             IfreqIntField which) const = 0;
  virtual std::optional<std::pair<std::string, int>>
  query_ifreq_sockaddr(const std::string &ifname, unsigned long req) const = 0;
  virtual std::vector<std::string>
  query_interface_groups(const std::string &ifname) const = 0;
  virtual void populateInterfaceMetadata(InterfaceConfig &ic) const = 0;

  // VRF matching helper
  virtual bool matches_vrf(const InterfaceConfig &ic,
                           const std::optional<VRFConfig> &vrf) const = 0;
};
