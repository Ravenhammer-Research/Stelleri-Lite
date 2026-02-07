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

#include "BridgeTableFormatter.hpp"
#include "BridgeInterfaceConfig.hpp"
#include "InterfaceConfig.hpp"
#include "InterfaceFlags.hpp"
#include "InterfaceType.hpp"
#include <algorithm>
#include <cstdio>
#include <iomanip>
#include <regex>
#include <sstream>
#include <vector>

std::string
BridgeTableFormatter::format(const std::vector<BridgeInterfaceConfig> &interfaces) const {
  if (interfaces.empty())
    return "No bridge interfaces found.\n";

  addColumn("Interface", "Interface", 10, 4, true);
  addColumn("STP", "STP", 6, 3, true);
  addColumn("VLANFiltering", "VLANFiltering", 5, 3, true);
  addColumn("Priority", "Priority", 4, 3, false);
  addColumn("Members", "Members", 3, 6, true);
  addColumn("MTU", "MTU", 4, 3, false);
  addColumn("Flags", "Flags", 3, 3, true);

  for (const auto &br : interfaces) {
    if (br.type != InterfaceType::Bridge)
      continue;

    std::string stp = br.stp ? "yes" : "no";
    std::string vlanf = br.vlanFiltering ? "yes" : "no";
    std::string prio =
        br.priority ? std::to_string(*br.priority) : std::string("-");
    std::string mtu = br.mtu ? std::to_string(*br.mtu) : std::string("-");
    std::string flags =
        (br.flags ? flagsToString(*br.flags) : std::string("-"));

    std::string membersCell = "-";
    if (!br.members.empty()) {
      std::ostringstream moss;
      for (size_t i = 0; i < br.members.size(); ++i) {
        if (i)
          moss << '\n';
        moss << br.members[i];
      }
      membersCell = moss.str();
    }

    addRow({br.name, stp, vlanf, prio, membersCell, mtu, flags});
  }

  return renderTable(80);
}
