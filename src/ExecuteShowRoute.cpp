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

#include "ConfigurationManager.hpp"
#include "RouteTableFormatter.hpp"
#include "RouteToken.hpp"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace netcli {

  void executeShowRoute(const RouteToken &tok, ConfigurationManager *mgr) {
    if (!mgr) {
      std::cout << "No ConfigurationManager provided\n";
      return;
    }
    // If a VRF token was provided, build a VRFConfig to request routes from
    // that routing table. Otherwise request global routes.
    std::optional<VRFConfig> vrfOpt = std::nullopt;
    if (tok.vrf) {
      VRFConfig v(tok.vrf->table());
      vrfOpt = std::move(v);
    }

    std::vector<RouteConfig> routes;
    // Retrieve RouteConfig entries for the requested VRF (or global).
    auto routeConfs = mgr->GetRoutes(vrfOpt);
    if (tok.prefix().empty()) {
      routes = std::move(routeConfs);
    } else {
      for (auto &rc : routeConfs) {
        if (rc.prefix == tok.prefix()) {
          routes.push_back(std::move(rc));
          break;
        }
      }
    }

    if (routes.empty()) {
      std::cout << "No routes found.\n";
      return;
    }

    std::string vrfContext = "Global";
    if (!routes.empty() && routes[0].vrf) {
      vrfContext = *routes[0].vrf;
    }

    RouteTableFormatter formatter;
    std::cout << formatter.format(routes);
  }
} // namespace netcli
