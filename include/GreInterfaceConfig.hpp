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
 * @file GREConfig.hpp
 * @brief GRE tunnel interface configuration
 */

#pragma once

#include "InterfaceConfig.hpp"
#include <optional>
#include <string>
#include <stdexcept>
#include "ConfigurationManager.hpp"

/**
 * @brief Configuration for GRE tunnel interfaces
 *
 * Wraps the FreeBSD gre(4) interface parameters accessible via
 * GRESADDRS/GRESADDRD/GRESKEY/GRESOPTS ioctls.
 */
class GreInterfaceConfig : public InterfaceConfig {
public:
  explicit GreInterfaceConfig(const InterfaceConfig &base) : InterfaceConfig(base) {}

  /// Tunnel source address
  std::optional<std::string> greSource;

  /// Tunnel destination (remote) address
  std::optional<std::string> greDestination;

  /// GRE key (0 = disabled)
  std::optional<uint32_t> greKey;

  /// GRE options bitmask (GRE_ENABLE_SEQ, GRE_ENABLE_CSUM, etc.)
  std::optional<uint32_t> greOptions;

  /// UDP encapsulation port (0 = no UDP encap)
  std::optional<uint16_t> grePort;

  /// Tunnel outer protocol family (AF_INET or AF_INET6, via GRESPROTO)
  std::optional<int> greProto;

  void save(ConfigurationManager &mgr) const override;
  void create(ConfigurationManager &mgr) const;
  void destroy(ConfigurationManager &mgr) const override;
};