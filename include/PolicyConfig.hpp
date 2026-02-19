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
 * @file PolicyConfig.hpp
 * @brief Policy configuration (access-lists, prefix-lists, route-maps)
 *
 * Currently supports access-list rules. Other policy types
 * (prefix-list, route, route6, route-map) are reserved for future use.
 */

#pragma once

#include "ConfigData.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

/**
 * @brief A single rule within an access-list
 *
 * Syntax: set policy access-list <N> rule <N> action <permit|deny>
 *         source <CIDR> destination <CIDR> protocol <proto>
 */
struct PolicyAccessListRule {
  uint32_t seq = 0;                       ///< Rule sequence number
  std::string action;                     ///< "permit" or "deny"
  std::optional<std::string> source;      ///< Source CIDR
  std::optional<std::string> destination; ///< Destination CIDR
  std::optional<std::string> protocol;    ///< Protocol: "any", "tcp", etc.
};

/**
 * @brief An access-list containing ordered rules
 */
struct PolicyAccessList {
  uint32_t id = 0; ///< Access-list number
  std::vector<PolicyAccessListRule> rules;
};

/**
 * @brief Configuration for a policy object
 *
 * The top-level "policy" noun dispatches to sub-types:
 *   access-list  — packet matching ACLs
 *   prefix-list  — prefix matching (future)
 *   route-map    — route redistribution (future)
 */
class PolicyConfig : public ConfigData {
public:
  enum class Type { AccessList /*, PrefixList, Route, Route6, RouteMap */ };

  Type policy_type = Type::AccessList;

  // ── Access-list fields ─────────────────────────────────────────────

  PolicyAccessList access_list;

  // ── ConfigData interface ───────────────────────────────────────────

  void save(ConfigurationManager &mgr) const override;
  void destroy(ConfigurationManager &mgr) const override;
};
