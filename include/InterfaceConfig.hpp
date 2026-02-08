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
 * @file InterfaceConfig.hpp
 * @brief Network interface configuration structure
 */

#pragma once

#include "ConfigData.hpp"
#include "IPNetwork.hpp"
#include "InterfaceType.hpp"
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "VRFConfig.hpp"
#include <string_view>

/**
 * @brief Complete configuration for a network interface
 *
 * Supports all interface types with type-specific configurations.
 * Optional fields allow for sparse configuration updates.
 */
class InterfaceConfig : public ConfigData {
public:
  InterfaceConfig() = default;
  // Platform-specific constructor removed; system layer builds instances.
  InterfaceConfig(std::string name, InterfaceType type,
                  std::unique_ptr<IPNetwork> address,
                  std::vector<std::unique_ptr<IPNetwork>> aliases,
                  std::unique_ptr<VRFConfig> vrf, std::optional<uint32_t> flags,
                  std::vector<std::string> groups, std::optional<int> mtu);
  // Copy semantics: deep copy owned pointers (defined out-of-line)
  InterfaceConfig(const InterfaceConfig &o);
  std::string name; ///< Interface name (e.g., em0, bridge0)
  InterfaceType type = InterfaceType::Unknown; ///< Interface type
  std::unique_ptr<IPNetwork> address; ///< Primary IP address with prefix
  std::vector<std::unique_ptr<IPNetwork>> aliases; ///< Additional IP addresses
  std::unique_ptr<VRFConfig> vrf;                  ///< VRF membership
  std::optional<uint32_t> flags;   ///< System flags (IFF_UP, IFF_RUNNING, etc.)
  std::vector<std::string> groups; ///< Interface groups
  std::optional<int> mtu;          ///< Maximum Transmission Unit
  std::optional<int> metric;       ///< Interface metric (if available)
  std::optional<int> index;        ///< Interface numeric index (if available)
  std::optional<std::string> nd6_options; ///< ND6 options string (if available)

  // --- Extended base-interface properties (from struct ifreq / ioctls) ---

  std::optional<std::string> description; ///< User description (SIOCGIFDESCR)
  std::optional<std::string> hwaddr; ///< Hardware / MAC address (SIOCGHWADDR)

  std::optional<uint32_t>
      capabilities; ///< Active HW caps – IFCAP_* (SIOCGIFCAP curcap)
  std::optional<uint32_t>
      req_capabilities; ///< Requested HW caps (SIOCGIFCAP reqcap)

  std::optional<std::string>
      media; ///< Current media type string (SIOCGIFMEDIA)
  std::optional<std::string>
      media_active; ///< Active media string (SIOCGIFMEDIA ifm_active)
  std::optional<int>
      media_status; ///< IFM_AVALID/IFM_ACTIVE (SIOCGIFMEDIA ifm_status)

  std::optional<std::string> status_str; ///< Driver status text (SIOCGIFSTATUS)
  std::optional<int> phys;               ///< Physical wire type (SIOCGIFPHYS)

  std::optional<uint64_t>
      baudrate; ///< Link speed in bits/sec (if_data.ifi_baudrate)
  std::optional<uint8_t> link_state; ///< Link state (if_data.ifi_link_state)

  // Persist this interface configuration via the supplied manager.
  void save(ConfigurationManager &mgr) const override;

  // Destroy this interface via the supplied manager.
  void destroy(ConfigurationManager &mgr) const override;

  // Remove an address from this interface (e.g., "192.0.2.1/32").
  void removeAddress(ConfigurationManager &mgr, const std::string &addr) const;

  // Check whether the named interface exists.
  static bool exists(const ConfigurationManager &mgr, std::string_view name);

  // Type checking predicates
  bool isBridge() const;
  bool isLagg() const;
  bool isVlan() const;
  bool isTunnelish() const;
  bool isVirtual() const;
  bool isWlan() const;
  bool isSixToFour() const;
  bool isTap() const;
  bool isCarp() const;
  bool isGre() const;
  bool isVxlan() const;
  bool isIpsec() const;

  // Check if this interface matches a requested type (handles tunnel special
  // cases)
  bool matchesType(InterfaceType requestedType) const;

  // Format a collection of interfaces using the appropriate formatter
  static std::string
  formatInterfaces(const std::vector<InterfaceConfig> &ifaces,
                   ConfigurationManager *mgr = nullptr);

protected:
  // (Interface existence check moved to `ConfigData::exists`)
};
