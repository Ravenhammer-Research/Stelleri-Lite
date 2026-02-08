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

#include "InterfaceTableFormatter.hpp"
#include "InterfaceConfig.hpp"
#include "InterfaceFlags.hpp"
#include "InterfaceType.hpp"
#include <algorithm>
#include <iomanip>
#include <net/if.h>
#include <sstream>

std::string InterfaceTableFormatter::format(
    const std::vector<InterfaceConfig> &interfaces) {
  if (interfaces.empty()) {
    return "No interfaces found.\n";
  }

  // Define columns (key, title, priority, minWidth)
  addColumn("Index", "Index", 8, 5, true);
  addColumn("Interface", "Interface", 10, 9, true);
  addColumn("Group", "Group", 6, 5, true);
  addColumn("Type", "Type", 9, 13, true);
  addColumn("Address", "Address", 10, 40, true);
  addColumn("Status", "Status", 7, 6, true);
  addColumn("MTU", "MTU", 5, 3, false);
  addColumn("VRF", "VRF", 4, 3, false);
  addColumn("Flags", "Flags", 3, 5, true);

  for (const auto &ic : interfaces) {

    std::string status = "-";
    if (ic.flags) {
      if (*ic.flags & IFF_RUNNING)
        status = "active";
      else if (*ic.flags & IFF_UP)
        status = "no-carrier";
      else
        status = "down";
    }

    std::string addrCell = "-";
    std::vector<std::string> addrLines;
    if (ic.address)
      addrLines.push_back(ic.address->toString());
    for (const auto &a : ic.aliases) {
      if (a)
        addrLines.push_back(a->toString());
    }
    if (!addrLines.empty()) {
      std::ostringstream aoss;
      for (size_t i = 0; i < addrLines.size(); ++i) {
        if (i)
          aoss << '\n';
        aoss << addrLines[i];
      }
      addrCell = aoss.str();
    }

    std::string mtuCell = ic.mtu ? std::to_string(*ic.mtu) : std::string("-");

    std::string vrfCell = "-";
    if (ic.vrf) {
      if (ic.vrf->table)
        vrfCell = std::to_string(ic.vrf->table);
      else
        vrfCell = std::to_string(ic.vrf->table);
    }

    std::string flagsCell =
        ic.flags ? flagsToString(*ic.flags) : std::string("-");

    std::string groupCell = "-";
    if (!ic.groups.empty()) {
      std::vector<std::string> filtered;
      for (const auto &g : ic.groups) {
        if (g == "all")
          continue;
        filtered.push_back(g);
      }
      if (!filtered.empty()) {
        std::ostringstream goss;
        for (size_t i = 0; i < filtered.size(); ++i) {
          if (i)
            goss << '\n';
          goss << filtered[i];
        }
        groupCell = goss.str();
      }
    }

    const std::string B = "\x1b[1m";
    const std::string R = "\x1b[0m";
    std::string indexCell = "-";
    if (ic.index)
      indexCell = B + std::to_string(*ic.index) + R;

    addRow({indexCell, ic.name, groupCell, interfaceTypeToString(ic.type),
            addrCell, status, mtuCell, vrfCell, flagsCell});
  }

  // Sort by index numeric ascending by default
  setSortColumn(0);

  const std::string B = "\x1b[1m";
  const std::string R = "\x1b[0m";
  std::string legend;
  legend += "Flags: ";
  legend += B + "U" + R + "=UP, ";
  legend += B + "B" + R + "=BROADCAST, ";
  legend += B + "D" + R + "=DEBUG, ";
  legend += B + "L" + R + "=LOOPBACK, ";
  legend += B + "P" + R + "=POINTOPOINT,";
  legend += "\n       ";
  legend += B + "e" + R + "=NEEDSEPOCH, ";
  legend += B + "R" + R + "=RUNNING, ";
  legend += B + "N" + R + "=NOARP, ";
  legend += B + "O" + R + "=PROMISC, ";
  legend += B + "p" + R + "=PPROMISC,";
  legend += "\n       ";
  legend += B + "A" + R + "=ALLMULTI, ";
  legend += B + "a" + R + "=PALLMULTI, ";
  legend += B + "M" + R + "=MULTICAST, ";
  legend += B + "s" + R + "=SIMPLEX, ";
  legend += B + "q" + R + "=OACTIVE,";
  legend += "\n       ";
  legend += B + "0/1/2" + R + "=LINK0/1/2, ";
  legend += B + "C" + R + "=CANTCONFIG, ";
  legend += B + "m" + R + "=MONITOR, ";
  legend += B + "x" + R + "=DYING, ";
  legend += B + "z" + R + "=RENAMING";
  legend += "\n\n";

  // Bold index numbers in rows: wrap when adding rows above. Call format
  auto out = renderTable(1000);
  out = legend + out;
  return out;
}
