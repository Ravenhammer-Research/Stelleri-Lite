#include "LoopBackTableFormatter.hpp"
#include "AbstractTableFormatter.hpp"
#include "ConfigData.hpp"
#include "InterfaceConfig.hpp"
#include <iomanip>
#include <sstream>

std::string
LoopBackTableFormatter::format(const std::vector<ConfigData> &items) const {
  AbstractTableFormatter atf;
  atf.addColumn("Interface", "Interface", 10, 4, true);
  atf.addColumn("Address", "Address", 5, 7, true);
  atf.addColumn("Status", "Status", 6, 6, true);

  for (const auto &cd : items) {
    if (!cd.iface)
      continue;
    const auto &ic = *cd.iface;
    if (ic.type != InterfaceType::Loopback)
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

    atf.addRow({ic.name, addrCell, status});
  }

  return atf.format(80);
}
