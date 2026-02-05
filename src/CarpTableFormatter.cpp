#include "CarpTableFormatter.hpp"
#include "AbstractTableFormatter.hpp"
#include "ConfigData.hpp"
#include "InterfaceConfig.hpp"
#include <sstream>

std::string
CarpTableFormatter::format(const std::vector<ConfigData> &items) const {
  AbstractTableFormatter atf;
  atf.addColumn("Interface", "Interface", 10, 4, true);
  atf.addColumn("Address", "Address", 5, 7, true);
  atf.addColumn("Status", "Status", 6, 6, true);
  atf.addColumn("MTU", "MTU", 6, 6, true);

  for (const auto &cd : items) {
    if (!cd.iface)
      continue;
    const auto &ic = *cd.iface;

    // Heuristic: CARP interfaces often start with "carp" or "vh"; treat virtual
    // types too
    if (ic.name.rfind("carp", 0) != 0 && ic.name.rfind("vh", 0) != 0 &&
        ic.type != InterfaceType::Virtual)
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

    std::string mtu = ic.mtu ? std::to_string(*ic.mtu) : std::string("-");

    atf.addRow({ic.name, addrCell, status, mtu});
  }

  return atf.format(80);
}
