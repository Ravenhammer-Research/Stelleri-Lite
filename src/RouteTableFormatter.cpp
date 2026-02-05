#include "RouteTableFormatter.hpp"
#include "AbstractTableFormatter.hpp"
#include "RouteConfig.hpp"
#include <iomanip>
#include <sstream>

std::string
RouteTableFormatter::format(const std::vector<ConfigData> &routes) const {
  if (routes.empty())
    return "No routes found.\n";

  // Determine VRF context (first route's VRF if present)
  std::string vrfContext = "Global";
  if (!routes.empty() && routes[0].route && routes[0].route->vrf)
    vrfContext = *routes[0].route->vrf;

  AbstractTableFormatter atf;
  atf.addColumn("Destination", "Destination", 8, 10, true);
  atf.addColumn("Gateway", "Gateway", 6, 7, true);
  atf.addColumn("Interface", "Interface", 6, 4, true);
  atf.addColumn("Flags", "Flags", 3, 2, true);

  for (const auto &cd : routes) {
    if (!cd.route)
      continue;
    const auto &route = *cd.route;

    std::string dest = route.prefix.empty() ? "-" : route.prefix;
    std::string gateway = route.nexthop.value_or("-");
    std::string iface = route.iface.value_or("-");
    std::string flags;
    if (route.blackhole)
      flags += "B";
    if (route.reject)
      flags += "R";
    if (!route.nexthop)
      flags += "C"; // Connected
    else if (!route.blackhole && !route.reject)
      flags += "UG"; // Up, Gateway

    atf.addRow({dest, gateway, iface, flags});
  }

  auto out = std::string("Routes (FIB: ") + vrfContext + ")\n\n";
  out += atf.format(80);
  return out;
}
