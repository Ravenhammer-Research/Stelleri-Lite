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

#include "PolicyToken.hpp"
#include "PolicyConfig.hpp"

// ── autoComplete ─────────────────────────────────────────────────────

std::vector<std::string>
PolicyToken::autoComplete(std::string_view partial) const {
  std::vector<std::string> options = {"access-list"};
  std::vector<std::string> matches;
  for (const auto &opt : options) {
    if (opt.rfind(partial, 0) == 0)
      matches.push_back(opt);
  }
  return matches;
}

// ── clone ────────────────────────────────────────────────────────────

std::unique_ptr<Token> PolicyToken::clone() const {
  auto t = std::make_unique<PolicyToken>();
  t->sub_type = sub_type;
  t->acl_id = acl_id;
  t->rule_seq = rule_seq;
  t->action = action;
  t->source = source;
  t->destination = destination;
  t->protocol = protocol;
  return t;
}

// ── toString (for config generation) ─────────────────────────────────

std::string PolicyToken::toString(const PolicyConfig *cfg) {
  if (!cfg)
    return std::string();

  std::string result;
  switch (cfg->policy_type) {
  case PolicyConfig::Type::AccessList: {
    const auto &acl = cfg->access_list;
    for (const auto &rule : acl.rules) {
      result += "policy access-list " + std::to_string(acl.id) + " rule " +
                std::to_string(rule.seq);
      if (!rule.action.empty())
        result += " action " + rule.action;
      if (rule.source)
        result += " source " + *rule.source;
      if (rule.destination)
        result += " destination " + *rule.destination;
      if (rule.protocol)
        result += " protocol " + *rule.protocol;
      result += "\n";
    }
    break;
  }
  }
  return result;
}

// ── parseAccessList ──────────────────────────────────────────────────

void PolicyToken::parseAccessList(const std::vector<std::string> &tokens,
                                  size_t &i,
                                  std::shared_ptr<PolicyToken> &tok) {
  tok->sub_type = SubType::AccessList;

  // Expect access-list number
  if (i < tokens.size()) {
    try {
      tok->acl_id = static_cast<uint32_t>(std::stoul(tokens[i]));
      ++i;
    } catch (...) {
      // Not a number — leave acl_id empty (show all)
    }
  }

  while (i < tokens.size()) {
    const std::string &kw = tokens[i];
    if (kw == "rule" && i + 1 < tokens.size()) {
      try {
        tok->rule_seq = static_cast<uint32_t>(std::stoul(tokens[i + 1]));
        i += 2;
      } catch (...) {
        break;
      }
    } else if (kw == "action" && i + 1 < tokens.size()) {
      tok->action = tokens[i + 1];
      i += 2;
    } else if (kw == "source" && i + 1 < tokens.size()) {
      tok->source = tokens[i + 1];
      i += 2;
    } else if (kw == "destination" && i + 1 < tokens.size()) {
      tok->destination = tokens[i + 1];
      i += 2;
    } else if (kw == "protocol" && i + 1 < tokens.size()) {
      tok->protocol = tokens[i + 1];
      i += 2;
    } else {
      break;
    }
  }
}

// ── parseFromTokens ──────────────────────────────────────────────────

std::shared_ptr<PolicyToken>
PolicyToken::parseFromTokens(const std::vector<std::string> &tokens,
                             size_t start, size_t &next) {
  auto tok = std::make_shared<PolicyToken>();
  next = start + 1; // consume the 'policy' keyword

  size_t i = next;
  if (i >= tokens.size()) {
    next = i;
    return tok;
  }

  const std::string &sub = tokens[i];
  if (sub == "access-list") {
    ++i;
    parseAccessList(tokens, i, tok);
  }
  // Future: else if (sub == "prefix-list") { ... }
  // Future: else if (sub == "route-map") { ... }

  next = i;
  return tok;
}
