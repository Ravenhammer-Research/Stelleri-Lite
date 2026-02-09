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

#include "NdpToken.hpp"
#include "NdpConfig.hpp"

NdpToken::NdpToken(std::string ip) : ip_(std::move(ip)) {}

// Static renderer for NdpConfig
std::string NdpToken::toString(NdpConfig *cfg) {
  if (!cfg) return std::string();
  std::string result = "ndp " + cfg->ip;
  if (!cfg->mac.empty()) result += " mac " + cfg->mac;
  if (cfg->iface) result += " interface " + *cfg->iface;
  if (cfg->permanent) result += " permanent";
  if (cfg->router) result += " router";
  if (cfg->expire) result += " expire " + std::to_string(*cfg->expire);
  return result;
}

std::vector<std::string> NdpToken::autoComplete(std::string_view partial) const {
  std::vector<std::string> options = {"mac", "interface", "permanent", "temp"};
  std::vector<std::string> matches;
  for (const auto &opt : options) {
    if (opt.rfind(partial, 0) == 0)
      matches.push_back(opt);
  }
  return matches;
}

std::unique_ptr<Token> NdpToken::clone() const {
  auto t = std::make_unique<NdpToken>(ip_);
  t->mac = mac;
  t->iface = iface;
  t->permanent = permanent;
  t->temp = temp;
  return t;
}

std::shared_ptr<NdpToken>
NdpToken::parseFromTokens(const std::vector<std::string> &tokens, size_t start,
                          size_t &next) {
  auto tok = std::make_shared<NdpToken>(std::string());
  next = start + 1; // consume the 'ndp' token

  size_t i = next;
  while (i < tokens.size()) {
    const std::string &kw = tokens[i];
    if (kw == "ip" && i + 1 < tokens.size()) {
      tok->ip_ = tokens[i + 1];
      i += 2;
    } else if (kw == "mac" && i + 1 < tokens.size()) {
      tok->mac = tokens[i + 1];
      i += 2;
    } else if (kw == "interface" && i + 1 < tokens.size()) {
      tok->iface = tokens[i + 1];
      i += 2;
    } else if (kw == "permanent") {
      tok->permanent = true;
      tok->temp = false;
      ++i;
    } else if (kw == "temp") {
      tok->temp = true;
      tok->permanent = false;
      ++i;
    } else {
      break; // unknown keyword, stop parsing
    }
  }

  next = i;
  return tok;
}
