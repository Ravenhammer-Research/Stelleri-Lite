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

#include "EpairTableFormatter.hpp"
#include "EpairInterfaceConfig.hpp"
#include "InterfaceConfig.hpp"
#include "InterfaceFlags.hpp"
#include "InterfaceType.hpp"
#include <map>
#include <optional>
#include <string>
#include <utility>

std::string
EpairTableFormatter::format(const std::vector<InterfaceConfig> &interfaces) {
  if (interfaces.empty())
    return "No epair interfaces found.\n";

  // Columns: Interface | Peer #1 | Peer #1 VRF | Peer #1 Status | Peer #2 |
  // Peer #2 VRF | Peer #2 Status
  addColumn("Interface", "Interface", 12, 9, true);
  addColumn("peer1", "Peer #1", 10, 7, true);
  addColumn("vrf1", "Peer #1 VRF", 6, 3, true);
  addColumn("status1", "Peer #1 Status", 8, 6, true);
  addColumn("peer2", "Peer #2", 10, 7, true);
  addColumn("vrf2", "Peer #2 VRF", 6, 3, true);
  addColumn("status2", "Peer #2 Status", 8, 6, true);

  // Build a map of pairs keyed by base name (strip trailing a/b)
  struct PairInfo {
    std::optional<InterfaceConfig> a;
    std::optional<InterfaceConfig> b;
  };

  std::map<std::string, PairInfo> pairs;

  for (const auto &ic : interfaces) {
    if (ic.type != InterfaceType::Epair)
      continue;
    std::string nm = ic.name;
    if (!nm.empty()) {
      char last = nm.back();
      if ((last == 'a' || last == 'b')) {
        std::string base = nm.substr(0, nm.size() - 1);
        auto &p = pairs[base];
        if (last == 'a')
          p.a.emplace(ic);
        else
          p.b.emplace(ic);
        continue;
      }
    }
    // not ending in a/b: put in 'a' slot by default
    pairs[nm].a.emplace(ic);
  }

  for (const auto &kv : pairs) {
    const auto &pi = kv.second;

    auto format_side = [](const std::optional<InterfaceConfig> &opt)
        -> std::pair<std::string, std::pair<std::string, std::string>> {
      if (!opt.has_value())
        return {"-", {"-", "-"}};
      const InterfaceConfig &ii = *opt;
      std::string vrf =
          ii.vrf ? std::to_string(ii.vrf->table) : std::string("-");
      std::string status = "-";
      if (ii.flags) {
        if (hasFlag(*ii.flags, InterfaceFlag::RUNNING))
          status = "active";
        else if (hasFlag(*ii.flags, InterfaceFlag::UP))
          status = "no-carrier";
        else
          status = "down";
      }
      return {ii.name, {vrf, status}};
    };

    auto [name_a, a_info] = format_side(pi.a);
    auto [name_b, b_info] = format_side(pi.b);

    addRow({kv.first, name_a, a_info.first, a_info.second, name_b, b_info.first,
            b_info.second});
  }

  return renderTable(1000) + "\n";
}
