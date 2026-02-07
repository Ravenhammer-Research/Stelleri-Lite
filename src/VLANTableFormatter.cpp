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

#include "VLANTableFormatter.hpp"
#include "InterfaceConfig.hpp"
#include "InterfaceFlags.hpp"
#include "InterfaceType.hpp"
#include "VLANConfig.hpp"
#include "VLANFlags.hpp"
#include "VLANProto.hpp"
#include <algorithm>
#include <iomanip>
#include <sstream>

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
VLANTableFormatter::format(const std::vector<InterfaceConfig> &interfaces) const {
  if (interfaces.empty()) {
    return "No VLAN interfaces found.\n";
  }

  std::ostringstream oss;

  // Table header (VLAN-specific: include VLAN name and flags)
  oss << std::left << std::setw(15) << "Interface" << std::setw(8) << "VLAN ID"
      << std::setw(12) << "Name" << std::setw(15) << "Parent" << std::setw(6)
      << "PCP" << std::setw(8) << "MTU" << std::setw(8) << "Flags"
      << "\n";
  oss << std::string(72, '-') << "\n";

  for (const auto &ic : interfaces) {
    if (ic.type != InterfaceType::VLAN)
      continue;

    // Try to use explicit VLAN payload if present; otherwise derive from name
    int vid = -1;
    std::string parent = "-";
    std::optional<int> pcp;
    const VLANConfig *vptr = dynamic_cast<const VLANConfig *>(&ic);
    if (vptr) {
      vid = vptr->id;
      parent = vptr->parent ? *vptr->parent : std::string("-");
      if (vptr->pcp)
        pcp = static_cast<int>(*vptr->pcp);
    } else {
      // Attempt to parse forms like "re0.25" (parent.name notation).
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
        (vptr ? vlanProtoToString(vptr->proto) : std::string("-"));
    std::string flagsStr =
        (ic.flags ? flagsToString(*ic.flags) : std::string("-"));
    std::string optionsStr = "-";
    if (vptr && vptr->options_bits) {
      auto s = vlanCapsToString(*vptr->options_bits);
      if (!s.empty())
        optionsStr = s;
    }

    // Add an Options column (capability flags from SIOCGIFCAP)
    oss << std::left << std::setw(15) << ic.name << std::setw(8) << vidStr
        << std::setw(12) << nameStr << std::setw(15) << parent << std::setw(6)
        << pcpStr << std::setw(8)
        << (ic.mtu ? std::to_string(*ic.mtu) : std::string("-")) << std::setw(8)
        << flagsStr << std::setw(12) << protoStr << std::setw(20) << optionsStr
        << "\n";
  }

  oss << "\n";
  return oss.str();
}
