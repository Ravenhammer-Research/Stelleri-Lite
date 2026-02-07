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
#include "InterfaceConfig.hpp"
#include "InterfaceFlags.hpp"
#include "InterfaceType.hpp"
#include "LaggConfig.hpp"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <net/ethernet.h>
#include <net/if.h>
#include <net/if_lagg.h>
#include <sstream>

// Local helper to render member flag bits into a human-readable label.
static std::string laggFlagsToLabel(uint32_t f) {
  std::string s;
  if (f & LAGG_PORT_MASTER) {
    if (!s.empty())
      s += ',';
    s += "MASTER";
  }
  if (f & LAGG_PORT_STACK) {
    if (!s.empty())
      s += ',';
    s += "STACK";
  }
  if (f & LAGG_PORT_ACTIVE) {
    if (!s.empty())
      s += ',';
    s += "ACTIVE";
  }
  if (f & LAGG_PORT_COLLECTING) {
    if (!s.empty())
      s += ',';
    s += "COLLECTING";
  }
  if (f & LAGG_PORT_DISTRIBUTING) {
    if (!s.empty())
      s += ',';
    s += "DISTRIBUTING";
  }
  return s;
}

static std::string protocolToString(LaggProtocol proto) {
  switch (proto) {
  case LaggProtocol::LACP:
    return "LACP";
  case LaggProtocol::FAILOVER:
    return "Failover";
  case LaggProtocol::LOADBALANCE:
    return "Load Balance";
  case LaggProtocol::NONE:
    return "None";
  default:
    return "Unknown";
  }
}

std::string LaggTableFormatter::format(
    const std::vector<InterfaceConfig> &interfaces) const {
  if (interfaces.empty())
    return "No LAGG interfaces found.\n";

  // Build a compact table: Interface | Protocol | HashPolicy | Members | MTU |
  // Flags | Status
  size_t nameWidth = 9;  // "Interface"
  size_t protoWidth = 8; // "Protocol"
  size_t hashWidth = 10; // "HashPolicy"
  size_t membersWidth = 10;
  size_t mtuWidth = 3;
  size_t flagsWidth = 5;
  size_t statusWidth = 6;

  struct Row {
    std::string name;
    std::string proto;
    std::optional<uint32_t> hash;
    std::vector<std::string> hash_items;
    std::vector<std::string> members;
    std::vector<uint32_t> member_flag_bits;
    std::vector<std::string> member_flags;
    std::optional<int> mtu;
    std::optional<uint32_t> flags;
    std::string status;
  };

  std::vector<Row> rows;

  for (const auto &ic : interfaces) {
    const LaggConfig *laggPtr = dynamic_cast<const LaggConfig *>(&ic);

    if (!laggPtr)
      continue;

    Row r;
    r.name = ic.name;
    r.proto = protocolToString(laggPtr->protocol);
    r.hash = laggPtr->hash_policy;
    // split hash policy bits into items for multiline display (l2,l3,l4)
    if (r.hash) {
      uint32_t m = *r.hash;
      if (m & LAGG_F_HASHL2)
        r.hash_items.emplace_back("l2");
      if (m & LAGG_F_HASHL3)
        r.hash_items.emplace_back("l3");
      if (m & LAGG_F_HASHL4)
        r.hash_items.emplace_back("l4");
    }
    r.members = laggPtr->members;
    // Prefer numeric flag bits when available; keep string labels as fallback.
    r.member_flag_bits = laggPtr->member_flag_bits;
    r.member_flags = laggPtr->member_flags;
    r.mtu = ic.mtu;
    r.flags = ic.flags;
    if (ic.flags) {
      if (*ic.flags & IFF_RUNNING)
        r.status = "active";
      else if (*ic.flags & IFF_UP)
        r.status = "no-carrier";
      else
        r.status = "down";
    } else {
      r.status = "-";
    }

    nameWidth = std::max(nameWidth, r.name.length());
    protoWidth = std::max(protoWidth, r.proto.length());
    if (r.hash) {
      if (!r.hash_items.empty()) {
        for (const auto &hi : r.hash_items)
          hashWidth = std::max(hashWidth, hi.length());
      } else {
        hashWidth = std::max(hashWidth, size_t(3));
      }
    }
    for (const auto &m : r.members)
      membersWidth = std::max(membersWidth, m.length());
    // Compute max width for flags column from either bit labels or existing
    // labels
    for (const auto &mf_bits : r.member_flag_bits) {
      std::string lbl = laggFlagsToLabel(mf_bits);
      flagsWidth = std::max(flagsWidth, lbl.length());
    }
    for (const auto &mf : r.member_flags)
      flagsWidth = std::max(flagsWidth, mf.length());
    for (const auto &hi : r.hash_items)
      hashWidth = std::max(hashWidth, hi.length());
    if (r.mtu)
      mtuWidth = std::max(mtuWidth, std::to_string(*r.mtu).length());
    if (r.flags)
      flagsWidth = std::max(flagsWidth, flagsToString(*r.flags).length());
    statusWidth = std::max(statusWidth, r.status.length());

    rows.push_back(std::move(r));
  }

  // Add padding
  nameWidth += 2;
  protoWidth += 2;
  hashWidth += 2;
  membersWidth += 2;
  mtuWidth += 2;
  flagsWidth += 2;
  statusWidth += 2;

  addColumn("Interface", "Interface", 10, 4, true);
  addColumn("Protocol", "Protocol", 8, 4, true);
  addColumn("HashPolicy", "HashPolicy", 3, 3, true);
  addColumn("Members", "Members", 3, 6, true);
  addColumn("MTU", "MTU", 4, 3, false);
  addColumn("Flags", "Flags", 3, 3, true);
  addColumn("Status", "Status", 6, 6, true);

  for (const auto &r : rows) {
    // Combine hash items into multiline cell if present
    std::string hashCell = "-";
    if (r.hash) {
      if (!r.hash_items.empty()) {
        std::ostringstream hoss;
        for (size_t i = 0; i < r.hash_items.size(); ++i) {
          if (i)
            hoss << '\n';
          hoss << r.hash_items[i];
        }
        hashCell = hoss.str();
      } else {
        uint32_t m = *r.hash;
        std::string s;
        if (m & LAGG_F_HASHL2)
          s += "l2";
        if (m & LAGG_F_HASHL3) {
          if (!s.empty())
            s += ",";
          s += "l3";
        }
        if (m & LAGG_F_HASHL4) {
          if (!s.empty())
            s += ",";
          s += "l4";
        }
        if (s.empty())
          s = "-";
        hashCell = s;
      }
    }

    std::string membersCell = "-";
    if (!r.members.empty()) {
      std::ostringstream moss;
      for (size_t i = 0; i < r.members.size(); ++i) {
        if (i)
          moss << '\n';
        moss << r.members[i];
      }
      membersCell = moss.str();
    }

    std::string mtuCell = r.mtu ? std::to_string(*r.mtu) : std::string("-");

    std::string flagsCell = "-";
    if (!r.member_flag_bits.empty()) {
      std::string lbl = laggFlagsToLabel(r.member_flag_bits[0]);
      flagsCell = lbl.empty() ? std::string("-") : lbl;
    } else if (!r.member_flags.empty() && r.member_flags[0] != "-") {
      flagsCell = r.member_flags[0];
    } else if (r.flags) {
      flagsCell = flagsToString(*r.flags);
    }

    addRow(
        {r.name, r.proto, hashCell, membersCell, mtuCell, flagsCell, r.status});
  }

  return renderTable(80);
}
