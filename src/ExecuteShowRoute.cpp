#include "ConfigurationManager.hpp"
#include "Parser.hpp"
#include "RouteTableFormatter.hpp"
#include "RouteToken.hpp"
#include <iomanip>
#include <iostream>
#include <sstream>
#include <algorithm>

void netcli::Parser::executeShowRoute(const RouteToken &tok,
                                      ConfigurationManager *mgr) const {
  if (!mgr) {
    std::cout << "No ConfigurationManager provided\n";
    return;
  }
  // If a VRF token was provided, build a VRFConfig to request routes from
  // that routing table. Otherwise request global routes.
  std::optional<VRFConfig> vrfOpt = std::nullopt;
  if (tok.vrf) {
    VRFConfig v;
    std::string nm = tok.vrf->name();
    // If the VRF token is numeric, treat it as a FIB/table id.
    bool isnumeric = !nm.empty() &&
                     std::all_of(nm.begin(), nm.end(), [](char c) {
                       return c >= '0' && c <= '9';
                     });
    if (isnumeric) {
      int t = std::stoi(nm);
      v.table = t;
      v.name = std::string("fib") + std::to_string(t);
    } else {
      v.name = nm;
    }
    vrfOpt = std::move(v);
  }

  std::vector<ConfigData> routes;
  // Retrieve RouteConfig entries for the requested VRF (or global) and
  // convert them into ConfigData for the formatter.
  auto routeConfs = mgr->GetRoutes(vrfOpt);
  if (tok.prefix().empty()) {
    routes.reserve(routeConfs.size());
    for (auto &rc : routeConfs) {
      ConfigData cd;
      cd.route = std::make_shared<RouteConfig>(std::move(rc));
      routes.push_back(std::move(cd));
    }
  } else {
    for (auto &rc : routeConfs) {
      if (rc.prefix == tok.prefix()) {
        ConfigData cd;
        cd.route = std::make_shared<RouteConfig>(std::move(rc));
        routes.push_back(std::move(cd));
        break;
      }
    }
  }

  if (routes.empty()) {
    std::cout << "No routes found.\n";
    return;
  }

  std::string vrfContext = "Global";
  if (!routes.empty() && routes[0].route && routes[0].route->vrf) {
    vrfContext = *routes[0].route->vrf;
  }

  RouteTableFormatter formatter;
  std::cout << formatter.format(routes);
}
