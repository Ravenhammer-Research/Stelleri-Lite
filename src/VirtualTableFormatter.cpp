#include "VirtualTableFormatter.hpp"
#include "AbstractTableFormatter.hpp"
#include "InterfaceConfig.hpp"
#include "InterfaceFlags.hpp"
#include "InterfaceType.hpp"
#include "VirtualInterfaceConfig.hpp"
#include <algorithm>
#include <array>
#include <iomanip>
#include <map>
#include <optional>
#include <sstream>
#include <string>

std::string
VirtualTableFormatter::format(const std::vector<ConfigData> &interfaces) const {
  if (interfaces.empty())
    return "No virtual interfaces found.\n";

  AbstractTableFormatter atf;
  // Columns: peer_a | VRF | MTU | Status | Flags | peer_b | VRF | MTU | Status
  // | Flags
  atf.addColumn("peer_a", "peer_a", 12, 2, true);
  atf.addColumn("vrf_a", "VRF", 4, 1, false);
  atf.addColumn("mtu_a", "MTU", 5, 1, false);
  atf.addColumn("status_a", "Status", 6, 1, false);
  atf.addColumn("flags_a", "Flags", 6, 1, true);
  atf.addColumn("peer_b", "peer_b", 12, 2, true);
  atf.addColumn("vrf_b", "VRF", 4, 1, false);
  atf.addColumn("mtu_b", "MTU", 5, 1, false);
  atf.addColumn("status_b", "Status", 6, 1, false);
  atf.addColumn("flags_b", "Flags", 6, 1, true);

  // Build a map of pairs keyed by base name (strip trailing a/b when
  // applicable)
  struct PairInfo {
    std::optional<InterfaceConfig> a;
    std::optional<InterfaceConfig> b;
  };

  std::map<std::string, PairInfo> pairs;

  for (const auto &cd : interfaces) {
    if (!cd.iface || cd.iface->type != InterfaceType::Virtual)
      continue;
    const InterfaceConfig &ic = *cd.iface;
    std::string nm = ic.name;
    // detect trailing 'a' or 'b' (e.g., epair14a)
    if (!nm.empty()) {
      char last = nm.back();
      if ((last == 'a' || last == 'b')) {
        std::string base = nm.substr(0, nm.size() - 1);
        auto &p = pairs[base];
        if (last == 'a')
          p.a = ic;
        else
          p.b = ic;
        continue;
      }
    }
    // not ending in a/b: put in 'a' slot by default
    pairs[nm].a = ic;
  }

  for (const auto &kv : pairs) {
    const auto &pi = kv.second;

    auto format_side = [&](const std::optional<InterfaceConfig> &opt) {
      if (!opt.has_value())
        return std::array<std::string, 4>{"-", "-", "-", "-"};
      const InterfaceConfig &ii = *opt;
      std::string vrf = (ii.vrf && ii.vrf->table.has_value())
                            ? std::to_string(*ii.vrf->table)
                            : std::string("-");
      std::string mtu = ii.mtu ? std::to_string(*ii.mtu) : std::string("-");
      std::string status = "-";
      if (ii.flags) {
        uint32_t f = *ii.flags;
        if (f & static_cast<uint32_t>(InterfaceFlag::UP))
          status = "UP";
        else
          status = "DOWN";
      }
      std::string flags =
          ii.flags ? flagsToString(*ii.flags) : std::string("-");
      return std::array<std::string, 4>{vrf, mtu, status, flags};
    };

    std::string name_a = pi.a ? pi.a->name : std::string("-");
    auto a_side = format_side(pi.a);
    std::string name_b = pi.b ? pi.b->name : std::string("-");
    auto b_side = format_side(pi.b);

    atf.addRow({name_a, a_side[0], a_side[1], a_side[2], a_side[3], name_b,
                b_side[0], b_side[1], b_side[2], b_side[3]});
  }

  auto out = atf.format(80);
  out += "\n";
  return out;
}
