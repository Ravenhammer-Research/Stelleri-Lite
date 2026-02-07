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

#include "TunnelTableFormatter.hpp"
#include "InterfaceConfig.hpp"
#include "InterfaceFlags.hpp"
#include "InterfaceType.hpp"
#include "TunnelConfig.hpp"
#include <algorithm>
#include <cstdio>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

std::string
TunnelTableFormatter::format(const std::vector<InterfaceConfig> &interfaces) const {
  if (interfaces.empty()) {
    return "No tunnel interfaces found.\n";
  }

  addColumn("Interface", "Interface", 10, 4, true);
  addColumn("Source", "Source", 5, 6, true);
  addColumn("Destination", "Destination", 5, 6, true);
  addColumn("Flags", "Flags", 5, 3, true);
  addColumn("Metric", "Metric", 4, 3, false);
  addColumn("MTU", "MTU", 4, 3, false);
  addColumn("Groups", "Groups", 2, 6, true);
  addColumn("FIB", "FIB", 5, 3, false);
  addColumn("TunFIB", "TunFIB", 4, 3, false);
  addColumn("ND6Opts", "ND6Opts", 1, 6, true);

  for (const auto &ic : interfaces) {
    if (!(ic.type == InterfaceType::Tunnel ||
          ic.type == InterfaceType::Gif ||
          ic.type == InterfaceType::Tun))
      continue;

    std::string source = "-";
    std::string destination = "-";
    std::string flagsStr = "-";
    std::string metricStr = "-";
    std::string mtuStr = "-";
    std::string groupsCell = "-";
    std::string fibStr = "-";
    std::string tunnelFibStr = "-";
    std::string nd6Cell = "-";

    const TunnelConfig *tptr = dynamic_cast<const TunnelConfig *>(&ic);
    if (tptr) {
      if (tptr->source)
        source = tptr->source->toString();
      if (tptr->destination)
        destination = tptr->destination->toString();
    }

    if (ic.flags)
      flagsStr = flagsToString(*ic.flags);
    if (ic.metric)
      metricStr = std::to_string(*ic.metric);
    if (ic.mtu)
      mtuStr = std::to_string(*ic.mtu);

    if (!ic.groups.empty()) {
      std::ostringstream goss;
      for (size_t i = 0; i < ic.groups.size(); ++i) {
        if (i)
          goss << '\n';
        goss << ic.groups[i];
      }
      groupsCell = goss.str();
    }

    if (ic.vrf) {
      if (ic.vrf->table)
        fibStr = std::to_string(ic.vrf->table);
      else if (ic.vrf->table >= 0)
        fibStr = std::to_string(ic.vrf->table);
    }

    if (tptr && tptr->tunnel_vrf)
      tunnelFibStr = std::to_string(*tptr->tunnel_vrf);

    if (ic.nd6_options)
      nd6Cell = *ic.nd6_options;

    addRow({ic.name, source, destination, flagsStr, metricStr, mtuStr,
                groupsCell, fibStr, tunnelFibStr, nd6Cell});
  }

  auto out = renderTable(80);
  out += "\n";
  return out;
}
