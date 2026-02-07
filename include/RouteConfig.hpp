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
 * @file RouteConfig.hpp
 * @brief Route configuration structure
 */

#pragma once

#include "ConfigData.hpp"
#include <optional>
#include <string>

/**
 * @brief Configuration for a routing table entry
 */
class RouteConfig : public ConfigData {
public:
  std::string prefix;                 ///< Destination prefix in CIDR notation
  std::optional<std::string> nexthop; ///< Next-hop IP address
  std::optional<std::string> iface;   ///< Outgoing interface name
  std::optional<std::string> vrf;     ///< VRF name for route
  bool blackhole = false;             ///< Blackhole route (drop packets)
  bool reject = false;                ///< Reject route (send ICMP unreachable)
  std::optional<std::string>
      scope;                 ///< Optional scope/interface for scoped addresses
  std::optional<int> expire; ///< Optional expire time (seconds)
  unsigned int flags = 0;    ///< raw rtm_flags from kernel

  // Persist route configuration (no-op for now)
  void save() const override {}
};
