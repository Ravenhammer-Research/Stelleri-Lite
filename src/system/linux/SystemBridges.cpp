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
#include <linux/sockios.h>
#include <net/if.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

/* Linux bridge-utils legacy ioctl interface */
#ifndef BRCTL_GET_VERSION
#define BRCTL_GET_VERSION 0
#define BRCTL_GET_BRIDGES 1
#define BRCTL_ADD_BRIDGE 2
#define BRCTL_DEL_BRIDGE 3
#define BRCTL_ADD_IF 4
#define BRCTL_DEL_IF 5
#define BRCTL_GET_BRIDGE_INFO 6
#define BRCTL_GET_PORT_LIST 7
#define BRCTL_SET_BRIDGE_FORWARD_DELAY 8
#define BRCTL_SET_BRIDGE_HELLO_TIME 9
#define BRCTL_SET_BRIDGE_MAX_AGE 10
#define BRCTL_SET_BRIDGE_PATH_COST 11
#define BRCTL_SET_BRIDGE_PRIORITY 12
#define BRCTL_SET_PORT_PRIORITY 13
#define BRCTL_SET_PATH_COST 14
#define BRCTL_GET_FDB_ENTRIES 15
#endif

std::vector<BridgeInterfaceConfig>
SystemConfigurationManager::GetBridgeInterfaces(
    const std::vector<InterfaceConfig> &bases) const {
  std::vector<BridgeInterfaceConfig> out;
  for (const auto &ic : bases) {
    if (ic.type == InterfaceType::Bridge) {
      BridgeInterfaceConfig bic(ic);
      bic.members = GetBridgeMembers(ic.name);
      out.emplace_back(std::move(bic));
    }
  }
  return out;
}

std::vector<std::string>
SystemConfigurationManager::GetBridgeMembers(const std::string &name) const {
  std::vector<std::string> members;
  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0)
    return members;

  unsigned long args[4];
  int indices[1024];

  args[0] = BRCTL_GET_PORT_LIST;
  args[1] = (unsigned long)indices;
  args[2] = 1024;
  args[3] = 0;

  struct ifreq ifr{};
  std::strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ - 1);
  ifr.ifr_data = (char *)args;

  if (ioctl(sock, SIOCDEVPRIVATE, &ifr) >= 0) {
    int count = ioctl(sock, SIOCDEVPRIVATE, &ifr);
    for (int i = 0; i < count; i++) {
      char ifname[IFNAMSIZ];
      if (if_indextoname(indices[i], ifname)) {
        members.push_back(ifname);
      }
    }
  }

  close(sock);
  return members;
}

void SystemConfigurationManager::CreateBridge(const std::string &name) const {
  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0)
    return;
  ioctl(sock, SIOCBRADDBR, name.c_str());
  close(sock);
}

void SystemConfigurationManager::SaveBridge(const BridgeInterfaceConfig &bic
                                            [[maybe_unused]]) const {
  // Can use SIOCBRADDIF to add members
}
