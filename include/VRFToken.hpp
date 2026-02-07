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
 * @file VRFToken.hpp
 * @brief Virtual Routing and Forwarding (VRF) token
 */

#pragma once

#include "Token.hpp"
#include <optional>
#include <string>

/**
 * @brief Token representing a VRF (Virtual Routing and Forwarding) instance
 *
 * VRFs provide routing table isolation. Each VRF has a name and optional
 * routing table ID.
 */
class VRFToken : public Token {
public:
  /**
   * @brief Construct VRF token with name
   * @param name VRF instance name
   */
  explicit VRFToken(std::string name);

  /** @brief textual reconstruction removed */

  /** @brief Get autocomplete suggestions (none for VRF) */
  std::vector<std::string> autoComplete(std::string_view) const override;

  /** @brief Clone the token */
  std::unique_ptr<Token> clone() const override;

  /** @brief Get VRF name */
  const std::string &name() const { return name_; }

  /** @brief Optional routing table ID for FreeBSD setfib() */
  std::optional<int> table;

private:
  std::string name_;
};
