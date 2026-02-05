#include "TunnelTableFormatter.hpp"
#include "InterfaceConfig.hpp"
#include "InterfaceType.hpp"
#include "InterfaceFlags.hpp"
#include "TunnelConfig.hpp"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <cstdio>
#include <string>
#include <vector>

std::string
TunnelTableFormatter::format(const std::vector<ConfigData> &interfaces) const {
  if (interfaces.empty()) {
    return "No tunnel interfaces found.\n";
  }

  std::ostringstream oss;
  oss << "\nTunnel Interfaces\n";
  oss << std::string(80, '=') << "\n\n";

  // Table header
    // Columns (condensed): Interface, Source, Destination, Flags, Metric, MTU,
    // Groups, FIB, TunFIB, ND6Opts
    oss << std::left << std::setw(10) << "Interface" << std::setw(16)
      << "Source" << std::setw(16) << "Destination" << std::setw(8)
      << "Flags" << std::setw(6) << "Metric" << std::setw(6) << "MTU"
      << std::setw(12) << "Groups" << std::setw(6) << "FIB"
      << std::setw(8) << "TunFIB" << std::setw(20) << "ND6Opts" << "\n";
    oss << std::string(100, '-') << "\n";

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
    // TTL/TOS are not reliably exposed via a simple ioctl on all tunnel
    // implementations; avoid displaying these fields to prevent incorrect
    // or stale values.
    std::string flagsStr = "-";
    std::string metricStr = "-";
    std::string mtuStr = "-";
    std::string groupsStr = "-";
    std::string fibStr = "-";
    std::string tunnelFibStr = "-";
    std::string nd6Str = "-";

    if (ic.tunnel) {
      if (ic.tunnel->source)
        source = ic.tunnel->source->toString();
      if (ic.tunnel->destination)
        destination = ic.tunnel->destination->toString();
    }

    // Flags
    if (ic.flags)
      flagsStr = flagsToString(*ic.flags);

    // Metric (from ioctl-populated metadata)
    if (ic.metric)
      metricStr = std::to_string(*ic.metric);

    // MTU
    if (ic.mtu)
      mtuStr = std::to_string(*ic.mtu);

    // Groups
    if (!ic.groups.empty()) {
      std::ostringstream goss;
      for (size_t i = 0; i < ic.groups.size(); ++i) {
        if (i) goss << ',';
        goss << ic.groups[i];
      }
      groupsStr = goss.str();
    }

    // FIB (VRF table or name)
    if (ic.vrf) {
      if (ic.vrf->table)
        fibStr = std::to_string(*ic.vrf->table);
      else if (!ic.vrf->name.empty())
        fibStr = ic.vrf->name;
    }

    // Tunnel FIB
    if (ic.tunnel_vrf)
      tunnelFibStr = std::to_string(*ic.tunnel_vrf);

    // ND6 options (from ioctl-populated metadata)
    if (ic.nd6_options)
      nd6Str = *ic.nd6_options;

    // Prepare ND6 options as separate lines
    std::vector<std::string> nd6Lines;
    if (nd6Str != "-" && !nd6Str.empty()) {
      std::istringstream nis(nd6Str);
      std::string tok;
      while (std::getline(nis, tok, ',')) {
        // trim
        while (!tok.empty() && isspace((unsigned char)tok.front())) tok.erase(tok.begin());
        while (!tok.empty() && isspace((unsigned char)tok.back())) tok.pop_back();
        if (!tok.empty()) nd6Lines.push_back(tok);
      }
    }
    if (nd6Lines.empty()) nd6Lines.push_back("-");

    // Prepare groups as separate lines (one-per-line)
    std::vector<std::string> groupsLines;
    if (!ic.groups.empty()) {
      for (const auto &g : ic.groups) {
        std::string tok = g;
        while (!tok.empty() && isspace((unsigned char)tok.front())) tok.erase(tok.begin());
        while (!tok.empty() && isspace((unsigned char)tok.back())) tok.pop_back();
        if (!tok.empty()) groupsLines.push_back(tok);
      }
    }
    if (groupsLines.empty()) groupsLines.push_back("-");

    // Determine rows needed to show both groups and nd6 options vertically
    size_t rows = std::max(groupsLines.size(), nd6Lines.size());

    // Print primary + continuation rows, aligning groups and nd6 vertically
    for (size_t i = 0; i < rows; ++i) {
      const std::string &gcell = (i < groupsLines.size() ? groupsLines[i] : "");
      const std::string &nd6cell = (i < nd6Lines.size() ? nd6Lines[i] : "");

      if (i == 0) {
        // primary row: print full metadata
        oss << std::left << std::setw(10) << ic.name << std::setw(16) << source
            << std::setw(16) << destination << std::setw(8) << flagsStr
            << std::setw(7) << metricStr << std::setw(6) << mtuStr
            << std::setw(12) << gcell << std::setw(6) << fibStr
            << std::setw(8) << tunnelFibStr << std::setw(20) << nd6cell << "\n";
      } else {
        // continuation rows: only groups and nd6 cells
        oss << std::left << std::setw(10) << "" << std::setw(16) << ""
            << std::setw(16) << "" << std::setw(8) << "" << std::setw(7) << ""
            << std::setw(6) << "" << std::setw(12) << gcell << std::setw(6) << ""
            << std::setw(8) << "" << std::setw(20) << nd6cell << "\n";
      }
    }
  }

  oss << "\n";
  return oss.str();
}
