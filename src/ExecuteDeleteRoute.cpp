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
#include "IPNetwork.hpp"
#include "RouteConfig.hpp"
#include "RouteToken.hpp"
#include <iostream>

#include <arpa/inet.h>
#include <cstring>
#include <errno.h>
#include <netinet/in.h>

namespace netcli {

void executeDeleteRoute(const RouteToken &tok,
                                ConfigurationManager *mgr) {
  (void)mgr;
  RouteConfig rc;
  rc.prefix = tok.prefix();
  if (tok.nexthop)
    rc.nexthop = tok.nexthop->toString();
  if (tok.interface)
    rc.iface = tok.interface->name();
  if (tok.vrf)
    rc.vrf = tok.vrf->table();
  rc.blackhole = tok.blackhole;
  rc.reject = tok.reject;

  auto net = IPNetwork::fromString(rc.prefix);
  if (!net) {
    std::cout << "delete route: invalid prefix: " << rc.prefix << "\n";
    return;
  }

  try {
    rc.destroy();
    std::cout << "delete route: " << rc.prefix << " removed\n";
  } catch (const std::exception &e) {
    std::cout << "delete route: failed: " << e.what() << "\n";
  }
}
}
