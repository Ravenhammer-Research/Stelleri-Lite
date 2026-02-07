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

#include "RouteTableFormatter.hpp"
#include "RouteConfig.hpp"
#include <iomanip>
#include <sys/socket.h>
#include <net/route.h>
#include <sstream>

std::string
RouteTableFormatter::format(const std::vector<RouteConfig> &routes) const {
  if (routes.empty())
    return "No routes found.\n";

  // Determine VRF context (first route's VRF if present)
  std::string vrfContext = "Global";
  if (!routes.empty() && routes[0].vrf)
    vrfContext = *routes[0].vrf;

  addColumn("Destination", "Destination", 8, 10, true);
  addColumn("Gateway", "Gateway", 6, 7, true);
  addColumn("Interface", "Interface", 6, 4, true);
  addColumn("Flags", "Flags", 3, 2, true);
  addColumn("Scope", "Scope", 5, 6, true);
  addColumn("Expire", "Expire", 6, 8, true);

  for (const auto &route : routes) {
    std::string dest = route.prefix.empty() ? "-" : route.prefix;
    std::string gateway = route.nexthop.value_or("-");
    std::string iface = route.iface.value_or("-");
    std::string scope = route.scope.value_or("-");
    std::string expire = "-";
    if (route.expire)
      expire = std::to_string(*route.expire);

    // Build flags in netstat order: U G H S B R (plain letters — legend is
    // bold)
    std::string flags;
    if (route.flags & RTF_UP)
      flags += "U";
    if (route.flags & RTF_GATEWAY)
      flags += "G";
    if (route.flags & RTF_HOST)
      flags += "H";
    if (route.flags & RTF_STATIC)
      flags += "S";
    if (route.blackhole)
      flags += "B";
    if (route.reject)
      flags += "R";

    addRow({dest, gateway, iface, flags, scope, expire});
  }

  // Display VRF header: if kernel reported a fib name like "fibN", show
  // numeric VRF id. Treat global as VRF 0.
  std::string vrfLabel;
  if (vrfContext.rfind("fib", 0) == 0) {
    std::string num = vrfContext.substr(3);
    vrfLabel = std::string("VRF: ") + num;
  } else if (vrfContext == "Global") {
    vrfLabel = std::string("VRF: 0");
  } else {
    vrfLabel = std::string("VRF: ") + vrfContext;
  }
  auto out = std::string("Routes (") + vrfLabel + ")\n\n";
  // Legend for route flags (abbrev letters are bold)
  out += std::string("Flags: ") + "\x1b[1mU\x1b[0m=up, " +
         "\x1b[1mG\x1b[0m=gateway, " + "\x1b[1mH\x1b[0m=host, " +
         "\x1b[1mS\x1b[0m=static, " + "\x1b[1mB\x1b[0m=blackhole, " +
         "\x1b[1mR\x1b[0m=reject\n\n";
  out += renderTable(80);
  return out;
}
