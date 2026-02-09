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
 * @file InterfaceToken.hpp
 * @brief Parser token for interface set/show/delete commands
 */

#pragma once

#include <iostream>
#include <optional>
#include <string>
#include "InterfaceType.hpp"

#include "BridgeInterfaceConfig.hpp"
#include "BridgeMemberConfig.hpp"
#include "CarpInterfaceConfig.hpp"
#include "EpairInterfaceConfig.hpp"
#include "GifInterfaceConfig.hpp"
#include "GreInterfaceConfig.hpp"
#include "InterfaceConfig.hpp"
#include "IpsecInterfaceConfig.hpp"
#include "LaggInterfaceConfig.hpp"
#include "LoopBackInterfaceConfig.hpp"
#include "OvpnInterfaceConfig.hpp"
#include "SixToFourInterfaceConfig.hpp"
#include "TapInterfaceConfig.hpp"
#include "TunInterfaceConfig.hpp"
#include "VRFConfig.hpp"
#include "VlanInterfaceConfig.hpp"
#include "VxlanInterfaceConfig.hpp"
#include "WlanInterfaceConfig.hpp"
#include "Token.hpp"

class InterfaceToken : public Token {
public:
  InterfaceToken(InterfaceType t, std::string name);

  /**
   * @brief Render an InterfaceConfig to a command string
   */
  static std::string toString(InterfaceConfig *cfg);

  /* overloads for specific interface-related configs */
  static std::string toString(BridgeInterfaceConfig *cfg);
  static std::string toString(CarpInterfaceConfig *cfg);
  static std::string toString(GreInterfaceConfig *cfg);
  static std::string toString(LaggInterfaceConfig *cfg);
  static std::string toString(SixToFourInterfaceConfig *cfg);
  static std::string toString(TapInterfaceConfig *cfg);
  
  static std::string toString(TunInterfaceConfig *cfg);
  static std::string toString(GifInterfaceConfig *cfg);
  static std::string toString(OvpnInterfaceConfig *cfg);
  static std::string toString(IpsecInterfaceConfig *cfg);
  static std::string toString(VlanInterfaceConfig *cfg);
  static std::string toString(VxlanInterfaceConfig *cfg);
  static std::string toString(WlanInterfaceConfig *cfg);
  std::vector<std::string>
  autoComplete(std::string_view partial) const override;
  std::vector<std::string>
  autoComplete(const std::vector<std::string> &tokens, std::string_view partial,
               ConfigurationManager *mgr) const override;
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
  std::optional<LaggInterfaceConfig> lagg;
  std::optional<VlanInterfaceConfig> vlan;
  std::optional<TunInterfaceConfig> tun;
  std::optional<GifInterfaceConfig> gif;
  std::optional<OvpnInterfaceConfig> ovpn;
  std::optional<IpsecInterfaceConfig> ipsec;
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
  bool expect_type_value_ = false;
};
