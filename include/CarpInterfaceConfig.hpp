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
 * @file CarpConfig.hpp
 * @brief CARP (Common Address Redundancy Protocol) interface configuration
 */

#pragma once

#include "InterfaceConfig.hpp"
#include <optional>
#include <string>
#include "ConfigurationManager.hpp"
#include <stdexcept>

/**
 * @brief Configuration for CARP redundancy interfaces
 *
 * Wraps the FreeBSD CARP parameters from struct carpreq (ip_carp.h):
 * virtual-host ID, advertisement skew/base, passphrase key, and state.
 */
class CarpInterfaceConfig : public InterfaceConfig {
public:
  explicit CarpInterfaceConfig(const InterfaceConfig &base) : InterfaceConfig(base) {}

  /// Virtual Host ID (1–255)
  std::optional<int> vhid;

  /// Advertisement skew (0–240)
  std::optional<int> advskew;

  /// Advertisement interval in seconds
  std::optional<int> advbase;

  /// CARP passphrase / HMAC key (max CARP_KEY_LEN = 20 bytes)
  std::optional<std::string> key;

  /// Current CARP state as string: INIT, BACKUP, or MASTER
  std::optional<std::string> state;

  void save(ConfigurationManager &mgr) const override;
  void create(ConfigurationManager &mgr) const;
  void destroy(ConfigurationManager &mgr) const override;
};
// implementations in src/cfg/CarpInterfaceConfig.cpp
