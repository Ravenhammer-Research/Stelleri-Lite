/*
 * Copyright (c) 2026, Ravenhammer Research Inc.
 * All rights reserved.
 */

#include "CommandGenerator.hpp"
#include "RouteConfig.hpp"
#include "RouteToken.hpp"
#include <iostream>

namespace netcli {

  void CommandGenerator::generateRoutes(ConfigurationManager &mgr) {
    // Prefer backend-provided static routes if available (implementation may
    // return only user-configured/static entries). Fall back to GetRoutes()
    // if GetStaticRoutes is not implemented by the backend.
    // Request all routes from the manager, then apply our DYNAMIC-only
    // filter below. `GetStaticRoutes()` would already restrict to static
    // entries and make a DYNAMIC filter ineffective.
    auto routes = mgr.GetRoutes();
    for (const auto &r : routes) {
      // Temporary debug: print route prefix, flags and nexthop to investigate
      // why the DYNAMIC-only filter emits nothing.
      // Decode flags into readable yes/no pairs for debugging.
      unsigned int f = r.flags;
      auto has = [&](RouteConfig::RouteFlag fl) {
        return (f & RouteConfig::Flag(fl)) != 0;
      };
      // std::ostringstream fd;
      // fd << "up=" << (has(RouteConfig::UP) ? "yes" : "no") << " ";
      // fd << "gateway=" << (has(RouteConfig::GATEWAY) ? "yes" : "no") << " ";
      // fd << "host=" << (has(RouteConfig::HOST) ? "yes" : "no") << " ";
      // fd << "reject=" << (has(RouteConfig::REJECT) ? "yes" : "no") << " ";
      // fd << "dynamic=" << (has(RouteConfig::DYNAMIC) ? "yes" : "no") << " ";
      // fd << "modified=" << (has(RouteConfig::MODIFIED) ? "yes" : "no") << " ";
      // fd << "done=" << (has(RouteConfig::DONE) ? "yes" : "no") << " ";
      // fd << "xresolve=" << (has(RouteConfig::XRESOLVE) ? "yes" : "no") << " ";
      // fd << "lldata=" << (has(RouteConfig::LLDATA) ? "yes" : "no") << " ";
      // fd << "static=" << (has(RouteConfig::STATIC) ? "yes" : "no") << " ";
      // fd << "blackhole=" << (has(RouteConfig::BLACKHOLE) ? "yes" : "no") << " ";
      // fd << "proto2=" << (has(RouteConfig::PROTO2) ? "yes" : "no") << " ";
      // fd << "proto1=" << (has(RouteConfig::PROTO1) ? "yes" : "no") << " ";
      // fd << "proto3=" << (has(RouteConfig::PROTO3) ? "yes" : "no") << " ";
      // fd << "fixedmtu=" << (has(RouteConfig::FIXEDMTU) ? "yes" : "no") << " ";
      // fd << "pinned=" << (has(RouteConfig::PINNED) ? "yes" : "no") << " ";
      // fd << "local=" << (has(RouteConfig::LOCAL) ? "yes" : "no") << " ";
      // fd << "broadcast=" << (has(RouteConfig::BROADCAST) ? "yes" : "no") << " ";
      // fd << "multicast=" << (has(RouteConfig::MULTICAST) ? "yes" : "no") << " ";
      // fd << "sticky=" << (has(RouteConfig::STICKY) ? "yes" : "no");

      // std::cerr << "[debug] route prefix='" << r.prefix << "' flags=0x"
      //           << std::hex << r.flags << std::dec
      //           << " nexthop='" << (r.nexthop ? *r.nexthop : std::string("-"))
      //           << "' iface='" << (r.iface ? *r.iface : std::string("-"))
      //           << "' " << fd.str() << "\n";
      
      // `PINNED` corresponds to kernel `RTF_PINNED` (see /usr/include/net/route.h)
      // - Meaning: the route is pinned/immutable (kernel marks it as not
      //   removable or modifiable by normal route operations).
      // - Intent: skip pinned routes here because they are system-managed
      //   infrastructure entries (not user-configured) and shouldn't be
      //   emitted as CLI 'set route' commands.
      if (has(RouteConfig::PINNED)) {
        continue;
      }

      std::cout << std::string("set ") +
                       RouteToken::toString(const_cast<RouteConfig *>(&r))
                << "\n";
    }
  }

} // namespace netcli
