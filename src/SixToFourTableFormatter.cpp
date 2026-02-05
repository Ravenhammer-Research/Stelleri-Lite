#include "SixToFourTableFormatter.hpp"
#include "AbstractTableFormatter.hpp"
#include "ConfigData.hpp"
#include "InterfaceConfig.hpp"
#include <sstream>

std::string
SixToFourTableFormatter::format(const std::vector<ConfigData> &items) const {
  AbstractTableFormatter atf;
  atf.addColumn("Interface", "Interface", 10, 4, true);
  atf.addColumn("Address", "Address", 5, 7, true);
  atf.addColumn("Status", "Status", 6, 6, true);
  atf.addColumn("VRF", "VRF", 4, 3, true);

  for (const auto &cd : items) {
    if (!cd.iface)
      continue;
    const auto &ic = *cd.iface;

    // Heuristic: treat Tunnel-type interfaces whose name starts with "gif" or
    // "stf" as 6to4/SIT-like
    if (ic.type != InterfaceType::Tunnel && ic.type != InterfaceType::Gif)
      continue;
    if (!(ic.name.rfind("gif", 0) == 0 || ic.name.rfind("stf", 0) == 0 ||
          ic.name.rfind("sit", 0) == 0))
      continue;

    std::vector<std::string> addrs;
    if (ic.address)
      addrs.push_back(ic.address->toString());
    for (const auto &a : ic.aliases) {
      if (a)
        addrs.push_back(a->toString());
    }

    std::ostringstream aoss;
    for (size_t i = 0; i < addrs.size(); ++i) {
      if (i)
        aoss << '\n';
      aoss << addrs[i];
    }
    std::string addrCell = addrs.empty() ? std::string("-") : aoss.str();

    std::string status = "-";
    if (ic.flags) {
      if (*ic.flags & IFF_RUNNING)
        status = "active";
      else if (*ic.flags & IFF_UP)
        status = "no-carrier";
      else
        status = "down";
    }

    std::string vrf = "-";
    if (ic.vrf && ic.vrf->table)
      vrf = std::to_string(*ic.vrf->table);

    atf.addRow({ic.name, addrCell, status, vrf});
  }

  return atf.format(80);
}
