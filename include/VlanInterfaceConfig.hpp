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
 * @file VLANConfig.hpp
 * @brief VLAN interface configuration
 */

#pragma once

#include "InterfaceConfig.hpp"
#include "PriorityCodePoint.hpp"
#include "VlanProto.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include "ConfigurationManager.hpp"
#include <stdexcept>

/**
 * @brief Configuration for VLAN interfaces
 *
 * Stores VLAN ID and parent interface relationship.
 */
class VlanInterfaceConfig : public InterfaceConfig {
public:
  VlanInterfaceConfig() = default;
  VlanInterfaceConfig(const InterfaceConfig &base);
  VlanInterfaceConfig(const InterfaceConfig &base, uint16_t id,
             std::optional<std::string> parent,
             std::optional<PriorityCodePoint> pcp);

  uint16_t id = 0; ///< VLAN ID (1-4094)
  std::optional<std::string>
      parent; ///< Parent interface (e.g., "em0" for em0.100)
  std::optional<PriorityCodePoint> pcp; ///< Priority Code Point (0-7)
  std::optional<VLANProto> proto;       ///< VLAN protocol as enum
  std::optional<uint32_t>
      options_bits; ///< Raw interface capability bits (IFCAP_*)

  void save(ConfigurationManager &mgr) const override;

  void destroy(ConfigurationManager &mgr) const override;

private:
  void create(ConfigurationManager &mgr) const;
};
// implementations in src/cfg/VlanInterfaceConfig.cpp
