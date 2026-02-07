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
#include <ifaddrs.h>
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
  InterfaceConfig(const struct ifaddrs *ifa);
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

  // Persist this interface configuration to the system.
  void save() const override;

  // Destroy this interface from the system.
  void destroy() const override;

  // Check whether the named interface exists on the system.
  static bool exists(std::string_view name);

protected:
  // Helper to configure addresses (primary + aliases) using the provided
  // socket and `ifr`. Performs necessary SIOCSIFADDR / SIOCSIFNETMASK calls.
  void configureAddresses(int sock, struct ifreq &ifr) const;

  // Helper to configure MTU using the provided socket and `ifr`.
  void configureMTU(int sock, struct ifreq &ifr) const;

  // Helper to configure flags (bring interface up) using the provided socket
  // and `ifr`.
  void configureFlags(int sock, struct ifreq &ifr) const;

  // (Interface existence check moved to `ConfigData::exists`)
};
