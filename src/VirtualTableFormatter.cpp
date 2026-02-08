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

#include "VirtualTableFormatter.hpp"
#include "InterfaceConfig.hpp"
#include "InterfaceFlags.hpp"
#include "InterfaceType.hpp"
#include "VirtualInterfaceConfig.hpp"
#include <algorithm>
#include <array>
#include <iomanip>
#include <map>
#include <optional>
#include <sstream>
#include <string>

std::string VirtualTableFormatter::format(
    const std::vector<InterfaceConfig> &interfaces) {
  if (interfaces.empty())
    return "No virtual interfaces found.\n";

  // Columns: peer_a | VRF | MTU | Status | Flags | peer_b | VRF | MTU | Status
  // | Flags
  addColumn("peer_a", "peer_a", 12, 2, true);
  addColumn("vrf_a", "VRF", 4, 1, false);
  addColumn("mtu_a", "MTU", 5, 1, false);
  addColumn("status_a", "Status", 6, 1, false);
  addColumn("flags_a", "Flags", 6, 1, true);
  addColumn("peer_b", "peer_b", 12, 2, true);
  addColumn("vrf_b", "VRF", 4, 1, false);
  addColumn("mtu_b", "MTU", 5, 1, false);
  addColumn("status_b", "Status", 6, 1, false);
  addColumn("flags_b", "Flags", 6, 1, true);

  // Build a map of pairs keyed by base name (strip trailing a/b when
  // applicable)
  struct PairInfo {
    std::optional<InterfaceConfig> a;
    std::optional<InterfaceConfig> b;
  };

  std::map<std::string, PairInfo> pairs;

  for (const auto &ic : interfaces) {
    if (ic.type != InterfaceType::Virtual)
      continue;
    std::string nm = ic.name;
    // detect trailing 'a' or 'b' (e.g., epair14a)
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

    auto format_side = [&](const std::optional<InterfaceConfig> &opt) {
      if (!opt.has_value())
        return std::array<std::string, 4>{"-", "-", "-", "-"};
      const InterfaceConfig &ii = *opt;
      std::string vrf =
          ii.vrf ? std::to_string(ii.vrf->table) : std::string("-");
      std::string mtu = ii.mtu ? std::to_string(*ii.mtu) : std::string("-");
      std::string status = "-";
      if (ii.flags) {
        uint32_t f = *ii.flags;
        if (f & static_cast<uint32_t>(InterfaceFlag::UP))
          status = "UP";
        else
          status = "DOWN";
      }
      std::string flags =
          ii.flags ? flagsToString(*ii.flags) : std::string("-");
      return std::array<std::string, 4>{vrf, mtu, status, flags};
    };

    std::string name_a = pi.a ? pi.a->name : std::string("-");
    auto a_side = format_side(pi.a);
    std::string name_b = pi.b ? pi.b->name : std::string("-");
    auto b_side = format_side(pi.b);

    addRow({name_a, a_side[0], a_side[1], a_side[2], a_side[3], name_b,
            b_side[0], b_side[1], b_side[2], b_side[3]});
  }

  auto out = renderTable(80);
  out += "\n";
  return out;
}
