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

#include "VRFToken.hpp"

VRFToken::VRFToken(int table) : table_(table) {}

// Static renderer for VRFConfig
std::string VRFToken::toString(VRFConfig *cfg) {
  if (!cfg) return std::string();
  return std::string("vrf ") + std::to_string(cfg->table);
}

std::vector<std::string> VRFToken::autoComplete(std::string_view) const {
  return {};
}

std::unique_ptr<Token> VRFToken::clone() const {
  return std::make_unique<VRFToken>(*this);
}

std::shared_ptr<VRFToken>
VRFToken::parseFromTokens(const std::vector<std::string> &tokens, size_t start,
                          size_t &next) {
  next = start + 1; // consume the 'vrf' token

  int table = 0;
  if (next < tokens.size()) {
    table = std::stoi(tokens[next]);
    ++next;
  }

  return std::make_shared<VRFToken>(table);
}
