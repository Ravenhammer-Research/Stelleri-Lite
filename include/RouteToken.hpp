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
 * @file RouteToken.hpp
 * @brief Parser token for route set/delete commands
 */

#pragma once

#include "ConfigurationManager.hpp"
#include "IPAddress.hpp"
#include "IPNetwork.hpp"
#include "InterfaceToken.hpp"
#include "Token.hpp"
#include "VRFToken.hpp"
#include <iomanip>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include "RouteConfig.hpp"

class RouteToken : public Token {
public:
  explicit RouteToken(std::string prefix);

  
  std::vector<std::string> autoComplete(std::string_view) const override;
  std::unique_ptr<Token> clone() const override;

  const std::string &prefix() const { return prefix_; }
  std::unique_ptr<IPAddress> nexthop;
  std::unique_ptr<InterfaceToken> interface;
  std::unique_ptr<VRFToken>
      vrf;                // only the id/name is completable in this context
  bool blackhole = false; // Kept for backward compatibility
  bool reject = false;    // Kept for backward compatibility

  void debugOutput(std::ostream &os) const;

  // Parse route tokens starting at `start` and return a RouteToken
  static std::shared_ptr<RouteToken>
  parseFromTokens(const std::vector<std::string> &tokens, size_t start,
                  size_t &next);

  // (Rendering moved to execute handlers; token is parse-only.)

private:
  std::string prefix_;
public:
  /**
   * @brief Render a RouteConfig to a command string
   */
  static std::string toString(RouteConfig *cfg);
};
