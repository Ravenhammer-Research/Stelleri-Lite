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

#include "SystemConfigurationManager.hpp"
#include "TunnelConfig.hpp"

#include <sys/sockio.h>

std::vector<TunnelConfig> SystemConfigurationManager::GetTunnelInterfaces(
    const std::optional<VRFConfig> &vrf) const {
  auto bases = GetInterfaces(vrf);
  std::vector<TunnelConfig> out;
  for (const auto &ic : bases) {
    if (ic.type == InterfaceType::Tunnel || ic.type == InterfaceType::Gif ||
        ic.type == InterfaceType::Tun) {
      TunnelConfig tconf(ic);

      if (auto tf = query_ifreq_int(ic.name, SIOCGTUNFIB, IfreqIntField::Fib);
          tf) {
        tconf.tunnel_vrf = *tf;
      }
      if (auto src = query_ifreq_sockaddr(ic.name, SIOCGIFPSRCADDR); src) {
        if (src->second == AF_INET)
          tconf.source = std::make_unique<IPv4Address>(src->first);
        else if (src->second == AF_INET6)
          tconf.source = std::make_unique<IPv6Address>(src->first);
      }
      if (auto dst = query_ifreq_sockaddr(ic.name, SIOCGIFPDSTADDR); dst) {
        if (dst->second == AF_INET)
          tconf.destination = std::make_unique<IPv4Address>(dst->first);
        else if (dst->second == AF_INET6)
          tconf.destination = std::make_unique<IPv6Address>(dst->first);
      }

      out.emplace_back(std::move(tconf));
    }
  }
  return out;
}
