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

#pragma once

#include "BridgeInterfaceConfig.hpp"
#include "ConfigurationManager.hpp"
#include "InterfaceType.hpp"
#include "LaggConfig.hpp"
#include "Token.hpp"
#include "TunnelConfig.hpp"
#include "VLANConfig.hpp"
#include <iostream>
#include <optional>
#include <string>

class InterfaceToken : public Token {
public:
  InterfaceToken(InterfaceType t, std::string name);

  std::string toString() const override;
  std::vector<std::string> autoComplete(std::string_view) const override;
  std::unique_ptr<Token> clone() const override;

  const std::string &name() const { return name_; }
  InterfaceType type() const { return type_; }
  std::optional<int> vrf;
  std::optional<std::string> group;
  std::optional<int> tunnel_vrf;
  std::optional<std::string> address;
  std::optional<int> address_family; // AF_INET or AF_INET6 when inet/inet6 used
  std::optional<int> mtu;
  std::optional<bool> status; // true=up, false=down
  std::optional<BridgeInterfaceConfig> bridge;
  std::optional<LaggConfig> lagg;
  std::optional<VLANConfig> vlan;
  std::optional<TunnelConfig> tunnel;

  // (Rendering moved to execute handlers. Token is parse-only.)

  // Parse an `interfaces` sequence starting at index `start` in `tokens`.
  // `start` should point to the "interfaces" token. On success, returns a
  // shared_ptr to an InterfaceToken and sets `next` to the index after consumed
  // tokens. Recognized forms:
  //  - interfaces
  //  - interfaces <type> <name>   where <type> is ethernet|loopback|pppoe
  static std::shared_ptr<InterfaceToken>
  parseFromTokens(const std::vector<std::string> &tokens, size_t start,
                  size_t &next);

private:
  /// Parse keyword arguments (inet, group, mtu, vrf, vlan, lagg) from tokens
  /// starting at cur, advancing cur past consumed tokens.
  static void parseKeywords(std::shared_ptr<InterfaceToken> &tok,
                            const std::vector<std::string> &tokens,
                            size_t &cur);

  InterfaceType type_ = InterfaceType::Unknown;
  std::string name_;
};
