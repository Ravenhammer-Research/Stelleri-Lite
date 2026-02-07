#include "WlanTableFormatter.hpp"
#include "AbstractTableFormatter.hpp"
#include "ConfigData.hpp"
#include "InterfaceConfig.hpp"
#include "WlanConfig.hpp"
#include "WlanAuthMode.hpp"
#include <sstream>

std::string
WlanTableFormatter::format(const std::vector<ConfigData> &items) const {
  AbstractTableFormatter atf;
  atf.addColumn("Interface", "Interface", 10, 4, true);
  atf.addColumn("SSID", "SSID", 20, 8, true);
  atf.addColumn("Channel", "Chan", 6, 3, true);
  atf.addColumn("Parent", "Parent", 10, 6, true);
  atf.addColumn("Status", "Status", 10, 6, true);
  atf.addColumn("Auth", "Auth", 6, 4, true);
  atf.addColumn("Address", "Address", 5, 7, true);
  atf.addColumn("MTU", "MTU", 6, 6, true);

  for (const auto &cd : items) {
    if (!cd.iface)
      continue;
    const auto &ic = *cd.iface;
    if (ic.type != InterfaceType::Wireless)
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
    std::string mtu = ic.mtu ? std::to_string(*ic.mtu) : std::string("-");
    std::string ssid = "-";
    std::string channel = "-";
    std::string parent = "-";
    std::string auth = "-";
    // If this InterfaceConfig is actually a WlanConfig, read wireless fields
    if (auto w = dynamic_cast<const WlanConfig *>(&ic)) {
      if (w->status)
        status = *w->status;
      else if (ic.flags) {
        if (*ic.flags & IFF_RUNNING)
          status = "active";
        else if (*ic.flags & IFF_UP)
          status = "no-carrier";
        else
          status = "down";
      }
      ssid = w->ssid ? *w->ssid : std::string("-");
      channel = w->channel ? std::to_string(*w->channel) : std::string("-");
      parent = w->parent ? *w->parent : std::string("-");
      if (w->authmode)
        auth = WlanAuthModeToString(*w->authmode);
    } else {
      if (ic.flags) {
        if (*ic.flags & IFF_RUNNING)
          status = "active";
        else if (*ic.flags & IFF_UP)
          status = "no-carrier";
        else
          status = "down";
      }
    }

    atf.addRow({ssid, channel, parent, status, auth, ic.name, addrCell, mtu});
  }

  return atf.format(80);
}
