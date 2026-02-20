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

#include "InterfaceConfig.hpp"
#include "RouteConfig.hpp"
#include "SystemConfigurationManager.hpp"

#include <cstring>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/route.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <unordered_map>

bool SystemConfigurationManager::matches_vrf(
    const InterfaceConfig &ic, const std::optional<VRFConfig> &vrf) const {
  if (!vrf)
    return true;
  if (!ic.vrf)
    return false;
  return ic.vrf->table == vrf->table;
}

std::vector<VRFConfig> SystemConfigurationManager::GetVrfs() const {
  std::vector<VRFConfig> out;
  int fibs = 1;
  size_t len = sizeof(fibs);
  if (sysctlbyname("net.fibs", &fibs, &len, nullptr, 0) != 0)
    fibs = 1;
  if (fibs <= 0)
    fibs = 1;
  for (int i = 0; i < fibs; ++i) {
    out.emplace_back(VRFConfig(i));
  }
  return out;
}
