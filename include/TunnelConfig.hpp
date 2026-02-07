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
 * @file TunnelConfig.hpp
 * @brief Tunnel interface configuration
 */

#pragma once

#include "IPAddress.hpp"
#include "InterfaceConfig.hpp"
#include "TunnelType.hpp"
#include <memory>
#include <optional>
#include <string>

/**
 * @brief Configuration for tunnel interfaces (GIF, GRE, etc.)
 */
/**
 * @brief Configuration for tunnel interfaces (GIF, GRE, etc.)
 */
class TunnelConfig : public InterfaceConfig {
public:
  TunnelConfig() = default;
  TunnelConfig(const InterfaceConfig &base);
  TunnelConfig(const InterfaceConfig &base, TunnelType type,
               std::unique_ptr<IPAddress> source,
               std::unique_ptr<IPAddress> destination);

  TunnelType type = TunnelType::UNKNOWN; ///< Tunnel implementation/type
  std::unique_ptr<IPAddress>
      source; ///< Tunnel source network/address (nullable)
  std::unique_ptr<IPAddress>
      destination; ///< Tunnel destination network/address (nullable)
  // TTL and TOS removed from TunnelConfig; managed elsewhere if needed
  std::optional<int> tunnel_vrf; ///< Tunnel-specific VRF/FIB (TunFIB)

  // Copy semantics: clone underlying IPNetwork objects
  TunnelConfig(const TunnelConfig &o)
      : InterfaceConfig(o), type(o.type), tunnel_vrf(o.tunnel_vrf) {
    if (o.source)
      source = o.source->clone();
    if (o.destination)
      destination = o.destination->clone();
  }

public:
  void save() const override;

private:
  void create() const;
};
