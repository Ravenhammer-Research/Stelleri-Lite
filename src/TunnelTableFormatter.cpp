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
#include "AbstractTableFormatter.hpp"
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
TunnelTableFormatter::format(const std::vector<ConfigData> &interfaces) const {
  if (interfaces.empty()) {
    return "No tunnel interfaces found.\n";
  }

  AbstractTableFormatter atf;
  atf.addColumn("Interface", "Interface", 10, 4, true);
  atf.addColumn("Source", "Source", 5, 6, true);
  atf.addColumn("Destination", "Destination", 5, 6, true);
  atf.addColumn("Flags", "Flags", 5, 3, true);
  atf.addColumn("Metric", "Metric", 4, 3, false);
  atf.addColumn("MTU", "MTU", 4, 3, false);
  atf.addColumn("Groups", "Groups", 2, 6, true);
  atf.addColumn("FIB", "FIB", 5, 3, false);
  atf.addColumn("TunFIB", "TunFIB", 4, 3, false);
  atf.addColumn("ND6Opts", "ND6Opts", 1, 6, true);

  for (const auto &cd : interfaces) {
    if (!cd.iface)
      continue;
    if (!(cd.iface->type == InterfaceType::Tunnel ||
          cd.iface->type == InterfaceType::Gif ||
          cd.iface->type == InterfaceType::Tun))
      continue;

    const auto &ic = *cd.iface;

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
        fibStr = std::to_string(*ic.vrf->table);
      else if (!ic.vrf->name.empty())
        fibStr = ic.vrf->name;
    }

    if (tptr && tptr->tunnel_vrf)
      tunnelFibStr = std::to_string(*tptr->tunnel_vrf);

    if (ic.nd6_options)
      nd6Cell = *ic.nd6_options;

    atf.addRow({ic.name, source, destination, flagsStr, metricStr, mtuStr,
                groupsCell, fibStr, tunnelFibStr, nd6Cell});
  }

  auto out = atf.format(80);
  out += "\n";
  return out;
}
