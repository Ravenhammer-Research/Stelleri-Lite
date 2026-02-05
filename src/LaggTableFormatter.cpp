#include "LaggTableFormatter.hpp"
#include "InterfaceConfig.hpp"
#include "InterfaceType.hpp"
#include "LaggConfig.hpp"
#include "InterfaceFlags.hpp"
#include <net/ethernet.h>
#include <net/if_lagg.h>
#include <net/if.h>
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>

// Local helper to render member flag bits into a human-readable label.
static std::string laggFlagsToLabel(uint32_t f) {
  std::string s;
  if (f & LAGG_PORT_MASTER) {
    if (!s.empty()) s += ',';
    s += "MASTER";
  }
  if (f & LAGG_PORT_STACK) {
    if (!s.empty()) s += ',';
    s += "STACK";
  }
  if (f & LAGG_PORT_ACTIVE) {
    if (!s.empty()) s += ',';
    s += "ACTIVE";
  }
  if (f & LAGG_PORT_COLLECTING) {
    if (!s.empty()) s += ',';
    s += "COLLECTING";
  }
  if (f & LAGG_PORT_DISTRIBUTING) {
    if (!s.empty()) s += ',';
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

std::string
LaggTableFormatter::format(const std::vector<ConfigData> &interfaces) const {
  if (interfaces.empty())
    return "No LAGG interfaces found.\n";

  // Build a compact table: Interface | Protocol | HashPolicy | Members | MTU | Flags | Status
  size_t nameWidth = 9;    // "Interface"
  size_t protoWidth = 8;   // "Protocol"
  size_t hashWidth = 10;   // "HashPolicy"
  size_t membersWidth = 10;
  size_t mtuWidth = 3;
  size_t flagsWidth = 5;
  size_t statusWidth = 6;

  struct Row { std::string name; std::string proto; std::optional<std::string> hash;
               std::vector<std::string> hash_items; std::vector<std::string> members;
               std::vector<uint32_t> member_flag_bits; std::vector<std::string> member_flags; std::optional<int> mtu;
               std::optional<uint32_t> flags; std::string status; };

  std::vector<Row> rows;

  for (const auto &cd : interfaces) {
    if (!cd.iface)
      continue;
    const auto &ic = *cd.iface;

    const LaggConfig *laggPtr = nullptr;
    if (ic.lagg)
      laggPtr = ic.lagg.get();
    else
      laggPtr = dynamic_cast<const LaggConfig *>(cd.iface.get());

    if (!laggPtr)
      continue;

    Row r;
    r.name = ic.name;
    r.proto = protocolToString(laggPtr->protocol);
    r.hash = laggPtr->hash_policy;
    // split hash policy into items for multiline display
    if (r.hash && !r.hash->empty()) {
      std::string tmp = *r.hash;
      size_t start = 0;
      while (start < tmp.size()) {
        size_t pos = tmp.find(',', start);
        if (pos == std::string::npos) pos = tmp.size();
        r.hash_items.emplace_back(tmp.substr(start, pos - start));
        start = pos + 1;
      }
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
    if (r.hash && !r.hash->empty())
      hashWidth = std::max(hashWidth, r.hash->length());
    for (const auto &m : r.members)
      membersWidth = std::max(membersWidth, m.length());
    // Compute max width for flags column from either bit labels or existing labels
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
  nameWidth += 2; protoWidth += 2; hashWidth += 2; membersWidth += 2;
  mtuWidth += 2; flagsWidth += 2; statusWidth += 2;

  std::ostringstream oss;
  // Header
  oss << std::left << std::setw(nameWidth) << "Interface"
      << std::setw(protoWidth) << "Protocol"
      << std::setw(hashWidth) << "HashPolicy"
      << std::setw(membersWidth) << "Members"
      << std::setw(mtuWidth) << "MTU"
      << std::setw(flagsWidth) << "Flags"
      << std::setw(statusWidth) << "Status"
      << "\n";

  // Separator
  oss << std::string(nameWidth, '-') << std::string(protoWidth, '-')
      << std::string(hashWidth, '-') << std::string(membersWidth, '-')
      << std::string(mtuWidth, '-') << std::string(flagsWidth, '-')
      << std::string(statusWidth, '-') << "\n";

  // Rows with multiline members and/or hash items
  for (const auto &r : rows) {
    size_t lines = std::max<size_t>(1, std::max(r.members.size(), r.hash_items.size()));
    for (size_t i = 0; i < lines; ++i) {
      if (i == 0) {
        oss << std::left << std::setw(nameWidth) << r.name
            << std::setw(protoWidth) << r.proto;
      } else {
        oss << std::left << std::setw(nameWidth) << "" << std::setw(protoWidth) << "";
      }

      // HashPolicy column: print the ith hash item if present
      if (i < r.hash_items.size())
        oss << std::setw(hashWidth) << r.hash_items[i];
      else if (i == 0)
        oss << std::setw(hashWidth) << (r.hash ? *r.hash : std::string("-"));
      else
        oss << std::setw(hashWidth) << "";

      // Members column
      if (i < r.members.size())
        oss << std::setw(membersWidth) << r.members[i];
      else if (i == 0)
        oss << std::setw(membersWidth) << "-";
      else
        oss << std::setw(membersWidth) << "";

      // MTU, Flags, Status only on first line
      if (i == 0) {
        if (r.mtu)
          oss << std::setw(mtuWidth) << *r.mtu;
        else
          oss << std::setw(mtuWidth) << "-";
        // Use first member's flag bits (preferred) or label fallback, otherwise interface flags
        if (!r.member_flag_bits.empty()) {
          std::string lbl = laggFlagsToLabel(r.member_flag_bits[0]);
          if (lbl.empty()) lbl = "-";
          oss << std::setw(flagsWidth) << lbl;
        } else if (!r.member_flags.empty() && r.member_flags[0] != "-") {
          oss << std::setw(flagsWidth) << r.member_flags[0];
        } else if (r.flags) {
          oss << std::setw(flagsWidth) << flagsToString(*r.flags);
        } else {
          oss << std::setw(flagsWidth) << "-";
        }
        oss << std::setw(statusWidth) << r.status;
      } else {
        oss << std::setw(mtuWidth) << "" << std::setw(flagsWidth) << "" << std::setw(statusWidth) << "";
      }

      oss << "\n";
    }
  }

  return oss.str();
}
