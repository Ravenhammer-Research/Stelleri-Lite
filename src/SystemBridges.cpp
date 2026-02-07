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

#include "BridgeInterfaceConfig.hpp"
#include "SystemConfigurationManager.hpp"

#include <cstring>
#include <net/if.h>
#include <net/if_bridgevar.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <unistd.h>

bool SystemConfigurationManager::interfaceIsBridge(
    const std::string &ifname) const {
  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0)
    return false;

  const size_t entries = 8;
  std::vector<struct ifbreq> buf(entries);
  std::memset(buf.data(), 0, buf.size() * sizeof(struct ifbreq));

  struct ifbifconf ifbic;
  std::memset(&ifbic, 0, sizeof(ifbic));
  ifbic.ifbic_len = static_cast<uint32_t>(buf.size() * sizeof(struct ifbreq));
  ifbic.ifbic_buf = reinterpret_cast<caddr_t>(buf.data());

  struct ifdrv ifd;
  std::memset(&ifd, 0, sizeof(ifd));
  std::strncpy(ifd.ifd_name, ifname.c_str(), IFNAMSIZ - 1);
  ifd.ifd_cmd = BRDGGIFS;
  ifd.ifd_len = static_cast<int>(sizeof(ifbic));
  ifd.ifd_data = &ifbic;

  bool res = false;
  if (ioctl(sock, SIOCGDRVSPEC, &ifd) == 0) {
    res = true;
  }

  close(sock);
  return res;
}

std::vector<BridgeInterfaceConfig>
SystemConfigurationManager::GetBridgeInterfaces(
    const std::optional<VRFConfig> &vrf) const {
  auto bases = GetInterfaces(vrf);
  std::vector<BridgeInterfaceConfig> out;
  for (const auto &ic : bases) {
    if (ic.type == InterfaceType::Bridge) {
      BridgeInterfaceConfig bic(ic);
      bic.loadMembers();
      out.emplace_back(std::move(bic));
    }
  }
  return out;
}
