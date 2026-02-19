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

#include "VlanTableFormatter.hpp"
#include "ConfigurationManager.hpp"
#include "InterfaceConfig.hpp"
#include "InterfaceFlags.hpp"
#include "InterfaceType.hpp"
#include "VlanInterfaceConfig.hpp"
#include "VlanFlags.hpp"
#include "VlanProto.hpp"
#include <algorithm>

static std::string vlanProtoToString(const std::optional<VLANProto> &p) {
  if (!p)
    return std::string("-");
  switch (*p) {
  case VLANProto::DOT1Q:
    return std::string("802.1q");
  case VLANProto::DOT1AD:
    return std::string("802.1ad");
  case VLANProto::UNKNOWN:
    return std::string("unknown");
  default:
    return std::string("other");
  }
}

// Convert raw IFCAP_* capability bits into a human-readable comma-separated
// string. Per project style this rendering belongs in the formatter.
static std::string vlanCapsToString(uint32_t mask) {
  std::string out;
  using netcli::VLANFlag;
  if (netcli::has_flag(mask, VLANFlag::RXCSUM)) {
    if (!out.empty())
      out += ",";
    out += "RXCSUM";
  }
  if (netcli::has_flag(mask, VLANFlag::TXCSUM)) {
    if (!out.empty())
      out += ",";
    out += "TXCSUM";
  }
  if (netcli::has_flag(mask, VLANFlag::LINKSTATE)) {
    if (!out.empty())
      out += ",";
    out += "LINKSTATE";
  }
  if (netcli::has_flag(mask, VLANFlag::VLAN_HWTAG)) {
    if (!out.empty())
      out += ",";
    out += "VLAN_HWTAG";
  }
  return out;
}

std::string
VlanTableFormatter::format(const std::vector<InterfaceConfig> &interfaces) {
  if (interfaces.empty()) {
    return "No VLAN interfaces found.\n";
  }

  // Re-query via the manager to get full VlanInterfaceConfig objects
  // (the input vector is sliced to base InterfaceConfig).
  std::vector<VlanInterfaceConfig> vlanIfaces;
  if (mgr_)
    vlanIfaces = mgr_->GetVLANInterfaces();

  // Define columns (key, title, priority, minWidth, leftAlign)
  addColumn("Interface", "Interface", 10, 9, true);
  addColumn("VLANID", "VLAN ID", 9, 7, true);
  addColumn("Name", "Name", 6, 4, true);
  addColumn("Parent", "Parent", 8, 6, true);
  addColumn("PCP", "PCP", 4, 3, true);
  addColumn("MTU", "MTU", 5, 3, true);
  addColumn("Flags", "Flags", 3, 5, true);
  addColumn("Proto", "Proto", 5, 5, true);
  addColumn("Options", "Options", 2, 7, true);

  for (const auto &ic : interfaces) {
    if (ic.type != InterfaceType::VLAN)
      continue;

    // Find matching VlanInterfaceConfig from the re-queried data
    const VlanInterfaceConfig *vptr = nullptr;
    for (const auto &v : vlanIfaces) {
      if (v.name == ic.name) {
        vptr = &v;
        break;
      }
    }

    int vid = -1;
    std::string parent = "-";
    std::optional<int> pcp;
    if (vptr) {
      vid = vptr->id;
      parent = vptr->parent ? *vptr->parent : std::string("-");
      if (vptr->pcp)
        pcp = static_cast<int>(*vptr->pcp);
    } else {
      // Fallback: parse forms like "re0.25" (parent.name notation).
      auto pos = ic.name.find('.');
      if (pos != std::string::npos) {
        parent = ic.name.substr(0, pos);
        std::string rest = ic.name.substr(pos + 1);
        try {
          vid = std::stoi(rest);
        } catch (...) {
          vid = -1;
        }
      }
    }

    std::string vidStr = (vid >= 0) ? std::to_string(vid) : std::string("-");
    std::string pcpStr =
        pcp ? std::to_string(static_cast<int>(*pcp)) : std::string("-");
    std::string nameStr = vptr ? vptr->name : std::string("-");
    std::string protoStr =
        vptr ? vlanProtoToString(vptr->proto) : std::string("-");
    std::string flagsStr =
        ic.flags ? flagsToString(*ic.flags) : std::string("-");
    std::string mtuStr =
        ic.mtu ? std::to_string(*ic.mtu) : std::string("-");
    std::string optionsStr = "-";
    if (vptr && vptr->options_bits) {
      auto s = vlanCapsToString(*vptr->options_bits);
      if (!s.empty())
        optionsStr = s;
    }

    addRow({ic.name, vidStr, nameStr, parent, pcpStr, mtuStr, flagsStr,
            protoStr, optionsStr});
  }

  // Sort by interface name
  setSortColumn(0);

  const std::string B = "\x1b[1m";
  const std::string R = "\x1b[0m";
  std::string legend;
  legend += "Flags: ";
  legend += B + "U" + R + "=UP, ";
  legend += B + "B" + R + "=BROADCAST, ";
  legend += B + "R" + R + "=RUNNING, ";
  legend += B + "M" + R + "=MULTICAST, ";
  legend += B + "s" + R + "=SIMPLEX";
  legend += "\n\n";

  auto out = renderTable(1000);
  out = legend + out;
  return out;
}
