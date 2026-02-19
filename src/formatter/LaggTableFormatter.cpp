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

#include "LaggTableFormatter.hpp"
#include "InterfaceFlags.hpp"
#include "LaggInterfaceConfig.hpp"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>

// Local helper to render member flag bits into a human-readable label.
static std::string laggFlagsToLabel(uint32_t f) {
  if (f == 0)
    return std::string();
  char buf[32];
  std::snprintf(buf, sizeof(buf), "0x%08x", f);
  return std::string(buf);
}

static std::string protocolToString(LaggProtocol proto) {
  switch (proto) {
  case LaggProtocol::LACP:
    return "LACP";
  case LaggProtocol::FAILOVER:
    return "Failover";
  case LaggProtocol::LOADBALANCE:
    return "Load Balance";
  case LaggProtocol::ROUNDROBIN:
    return "Round Robin";
  case LaggProtocol::BROADCAST:
    return "Broadcast";
  case LaggProtocol::NONE:
    return "None";
  default:
    return "Unknown";
  }
}

std::string
LaggTableFormatter::format(const std::vector<LaggInterfaceConfig> &interfaces) {
  if (interfaces.empty())
    return "No LAGG interfaces found.\n";

  addColumn("Interface", "Interface", 10, 4, true);
  addColumn("Protocol", "Protocol", 8, 4, true);
  addColumn("HashPolicy", "HashPolicy", 3, 3, true);
  addColumn("Members", "Members", 3, 6, true);
  addColumn("MTU", "MTU", 4, 3, false);
  addColumn("Flags", "Flags", 3, 3, true);
  addColumn("Status", "Status", 6, 6, true);

  for (const auto &lac : interfaces) {
    std::string proto = protocolToString(lac.protocol);

    std::string hashCell = "-";
    if (lac.hash_policy) {
      char hbuf[32];
      std::snprintf(hbuf, sizeof(hbuf), "0x%08x", *lac.hash_policy);
      hashCell = std::string(hbuf);
    }

    std::string membersCell = "-";
    if (!lac.members.empty()) {
      std::ostringstream moss;
      for (size_t i = 0; i < lac.members.size(); ++i) {
        if (i)
          moss << '\n';
        moss << lac.members[i];
      }
      membersCell = moss.str();
    }

    std::string mtuCell = lac.mtu ? std::to_string(*lac.mtu) : std::string("-");

    std::string flagsCell = "-";
    if (!lac.member_flag_bits.empty()) {
      std::string lbl = laggFlagsToLabel(lac.member_flag_bits[0]);
      flagsCell = lbl.empty() ? std::string("-") : lbl;
    } else if (!lac.member_flags.empty() && lac.member_flags[0] != "-") {
      flagsCell = lac.member_flags[0];
    } else if (lac.flags) {
      flagsCell = flagsToString(*lac.flags);
    }

    std::string status = "-";
    if (lac.flags) {
      if (hasFlag(*lac.flags, InterfaceFlag::RUNNING))
        status = "active";
      else if (hasFlag(*lac.flags, InterfaceFlag::UP))
        status = "no-carrier";
      else
        status = "down";
    }

    addRow(
        {lac.name, proto, hashCell, membersCell, mtuCell, flagsCell, status});
  }

  return renderTable(80);
}
