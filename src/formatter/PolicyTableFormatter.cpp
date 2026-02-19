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

#include "PolicyTableFormatter.hpp"
#include "PolicyConfig.hpp"
#include <iomanip>
#include <sstream>

std::string
PolicyTableFormatter::format(const std::vector<PolicyConfig> &entries) {
  if (entries.empty())
    return "No policy entries found.\n";

  addColumn("ACL", "ACL", 3, 5, true);
  addColumn("Seq", "Seq", 3, 5, true);
  addColumn("Action", "Action", 4, 6, true);
  addColumn("Source", "Source", 6, 18, true);
  addColumn("Destination", "Destination", 6, 18, true);
  addColumn("Protocol", "Protocol", 4, 6, true);

  for (const auto &entry : entries) {
    if (entry.policy_type != PolicyConfig::Type::AccessList)
      continue;
    const auto &acl = entry.access_list;
    for (const auto &rule : acl.rules) {
      addRow({std::to_string(acl.id), std::to_string(rule.seq),
              rule.action.empty() ? "-" : rule.action,
              rule.source.value_or("any"), rule.destination.value_or("any"),
              rule.protocol.value_or("any")});
    }
  }

  auto out = std::string("Access Lists\n\n");
  out += renderTable(120);
  return out;
}
