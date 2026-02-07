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
 * @file BridgeInterfaceConfig.hpp
 * @brief Bridge interface configuration structure
 */

#pragma once

#include "InterfaceConfig.hpp"

#include <optional>
#include <string>
#include <vector>

#include "BridgeMemberConfig.hpp"

/**
 * @brief Configuration for bridge interfaces
 *
 * Stores bridge-specific settings including STP and member interfaces.
 */
class BridgeInterfaceConfig : public InterfaceConfig {
public:
  BridgeInterfaceConfig() = default;
  BridgeInterfaceConfig(const InterfaceConfig &base);
  BridgeInterfaceConfig(const InterfaceConfig &base, bool stp,
                        bool vlanFiltering, std::vector<std::string> members,
                        std::vector<BridgeMemberConfig> member_configs,
                        std::optional<int> priority,
                        std::optional<int> hello_time,
                        std::optional<int> forward_delay,
                        std::optional<int> max_age,
                        std::optional<int> aging_time,
                        std::optional<int> max_addresses);

public:
  bool stp = false;                 ///< Spanning Tree Protocol enabled
  bool vlanFiltering = false;       ///< VLAN filtering enabled
  std::vector<std::string> members; ///< Member interface names (simple)
  std::vector<BridgeMemberConfig>
      member_configs;          ///< Detailed member configurations
  std::optional<int> priority; ///< Bridge priority (0-65535, default 32768)
  std::optional<int>
      hello_time; ///< STP hello time in seconds (1-10, default 2)
  std::optional<int>
      forward_delay; ///< STP forward delay in seconds (4-30, default 15)
  std::optional<int> max_age; ///< STP max age in seconds (6-40, default 20)
  std::optional<int>
      aging_time; ///< MAC address aging time in seconds (10-1000000)
  std::optional<int>
      max_addresses; ///< Maximum number of MAC addresses in cache

  // Persist bridge configuration to the system.
  void save() const override;

  // Populate runtime bridge data (member list, STP params) from the kernel.
  // This performs platform-specific ioctls and should be called on FreeBSD.
  void loadMembers();

private:
  // Ensure the bridge interface exists; create if necessary.
  void create() const;
};
