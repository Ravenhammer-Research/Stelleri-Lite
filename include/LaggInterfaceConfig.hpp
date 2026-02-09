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
 * @file LaggConfig.hpp
 * @brief Link aggregation (LAGG) configuration
 */

#pragma once

#include "InterfaceConfig.hpp"
#include <optional>
#include <string>
#include <vector>

#include "LaggProtocol.hpp"
#include "ConfigurationManager.hpp"

/**
 * @brief Configuration for link aggregation interfaces
 */
class LaggInterfaceConfig : public InterfaceConfig {
public:
  LaggInterfaceConfig() = default;
  LaggInterfaceConfig(const InterfaceConfig &base);
  LaggInterfaceConfig(const InterfaceConfig &base, LaggProtocol protocol,
             std::vector<std::string> members,
             std::optional<uint32_t> hash_policy, std::optional<int> lacp_rate,
             std::optional<int> min_links);
  LaggProtocol protocol =
      LaggProtocol::NONE;           ///< LAGG protocol (LACP, failover, etc.)
  std::vector<std::string> members; ///< Member port names
  std::vector<std::string>
      member_flags; ///< Per-member flag label (e.g., "MASTER")
  std::vector<uint32_t>
      member_flag_bits;                ///< Per-member raw flag bits from kernel
  std::optional<uint32_t> hash_policy; ///< Hash policy bitmask (L2/L3/L4)
  std::optional<int> lacp_rate;        ///< LACP rate: 0=slow (30s), 1=fast (1s)
  std::optional<int> min_links;        ///< Minimum number of active links
  std::optional<int> flowid_shift;     ///< Flow-ID hash shift (ro_flowid_shift)
  std::optional<uint32_t> rr_stride;   ///< Round-robin stride (ro_bkt)
  std::optional<int> options;          ///< Option bitmap (LAGG_OPT_*)
  std::optional<int> active_ports;     ///< Number of active ports (read-only)
  std::optional<int> flapping;         ///< Port flapping counter (read-only)

  ~LaggInterfaceConfig() override = default;
  void save(ConfigurationManager &mgr) const override;

private:
  void create(ConfigurationManager &mgr) const;
};

// implementations in src/cfg/LaggInterfaceConfig.cpp
