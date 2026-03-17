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

#include "LaggInterfaceConfig.hpp"
#include "Socket.hpp"
#include "SystemConfigurationManager.hpp"
#include <cstring>
#include <iostream>
#include <linux/if_link.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

namespace {

  // LaggProtocol mapping for Linux Bonding modes
  // Mode 0: balance-rr
  // Mode 1: active-backup
  // Mode 2: balance-xor
  // Mode 3: broadcast
  // Mode 4: 802.3ad (LACP)
  // Mode 5: balance-tlb
  // Mode 6: balance-alb

  LaggProtocol bondModeToLaggProtocol(int mode) {
    switch (mode) {
    case 0:
      return LaggProtocol::ROUNDROBIN;
    case 1:
      return LaggProtocol::FAILOVER;
    case 2:
      return LaggProtocol::LOADBALANCE;
    case 3:
      return LaggProtocol::BROADCAST;
    case 4:
      return LaggProtocol::LACP;
    default:
      return LaggProtocol::NONE;
    }
  }

  int laggProtocolToBondMode(LaggProtocol proto) {
    switch (proto) {
    case LaggProtocol::ROUNDROBIN:
      return 0;
    case LaggProtocol::FAILOVER:
      return 1;
    case LaggProtocol::LOADBALANCE:
      return 2;
    case LaggProtocol::BROADCAST:
      return 3;
    case LaggProtocol::LACP:
      return 4;
    default:
      return 0;
    }
  }

  // Netlink helpers (re-using patterns from SystemVrf.cpp)
  struct nl_msg {
    struct nlmsghdr n;
    struct ifinfomsg i;
    char buf[1024];
  };

  void add_attr(struct nlmsghdr *n, int type, const void *data, int len) {
    int nlalen = NLMSG_ALIGN(n->nlmsg_len);
    struct rtattr *rta = (struct rtattr *)(((char *)n) + nlalen);
    rta->rta_type = type;
    rta->rta_len = RTA_LENGTH(len);
    std::memcpy(RTA_DATA(rta), data, len);
    n->nlmsg_len = nlalen + RTA_ALIGN(rta->rta_len);
  }

  struct rtattr *begin_nest(struct nlmsghdr *n, int type) {
    struct rtattr *nest =
        (struct rtattr *)(((char *)n) + NLMSG_ALIGN(n->nlmsg_len));
    nest->rta_type = type;
    nest->rta_len = RTA_LENGTH(0);
    n->nlmsg_len = NLMSG_ALIGN(n->nlmsg_len) + RTA_ALIGN(nest->rta_len);
    return nest;
  }

  void end_nest(struct nlmsghdr *n, struct rtattr *nest) {
    nest->rta_len = (char *)n + n->nlmsg_len - (char *)nest;
  }

} // namespace

std::vector<LaggInterfaceConfig> SystemConfigurationManager::GetLaggInterfaces(
    const std::vector<InterfaceConfig> &bases) const {
  std::vector<LaggInterfaceConfig> results;
  int sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
  if (sock < 0)
    return results;

  for (const auto &base : bases) {
    if (base.type == InterfaceType::Lagg) {
      LaggInterfaceConfig lc(base);

      // Query detailed bond info
      struct {
        struct nlmsghdr n;
        struct ifinfomsg i;
      } req{};
      req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
      req.n.nlmsg_flags = NLM_F_REQUEST;
      req.n.nlmsg_type = RTM_GETLINK;
      req.i.ifi_family = AF_UNSPEC;
      req.i.ifi_index = base.index.value_or(0);

      if (send(sock, &req, req.n.nlmsg_len, 0) >= 0) {
        char buf[4096];
        ssize_t len = recv(sock, buf, sizeof(buf), 0);
        if (len > 0) {
          struct nlmsghdr *nh = (struct nlmsghdr *)buf;
          for (; NLMSG_OK(nh, static_cast<uint32_t>(len));
               nh = NLMSG_NEXT(nh, len)) {
            if (nh->nlmsg_type != RTM_NEWLINK)
              continue;
            struct ifinfomsg *ifi = (struct ifinfomsg *)NLMSG_DATA(nh);
            struct rtattr *rta = IFLA_RTA(ifi);
            int rta_len =
                static_cast<int>(nh->nlmsg_len - NLMSG_LENGTH(sizeof(*ifi)));
            for (; RTA_OK(rta, rta_len); rta = RTA_NEXT(rta, rta_len)) {
              if (rta->rta_type == IFLA_LINKINFO) {
                struct rtattr *li = (struct rtattr *)RTA_DATA(rta);
                int li_len = static_cast<int>(RTA_PAYLOAD(rta));
                for (; RTA_OK(li, li_len); li = RTA_NEXT(li, li_len)) {
                  if (li->rta_type == IFLA_INFO_DATA) {
                    struct rtattr *id = (struct rtattr *)RTA_DATA(li);
                    int id_len = static_cast<int>(RTA_PAYLOAD(li));
                    for (; RTA_OK(id, id_len); id = RTA_NEXT(id, id_len)) {
                      if (id->rta_type == IFLA_BOND_MODE) {
                        lc.protocol =
                            bondModeToLaggProtocol(*(uint8_t *)RTA_DATA(id));
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }

      // Identify members: Interfaces whose IFLA_MASTER matches this bond's
      // index This is easier if we iterate over all interfaces again or use a
      // cached map. For now, let's keep it simple.
      for (const auto &m : bases) {
        // In Linux, we'd need to query each interface for its master.
        // We'll skip for a moment to implement the rest.
      }

      results.push_back(std::move(lc));
    }
  }

  close(sock);
  return results;
}

void SystemConfigurationManager::CreateLagg(const std::string &name) const {
  if (name.empty())
    return;

  int sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
  if (sock < 0)
    return;

  struct nl_msg req{};
  req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
  req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL | NLM_F_ACK;
  req.n.nlmsg_type = RTM_NEWLINK;
  req.i.ifi_family = AF_UNSPEC;

  add_attr(&req.n, IFLA_IFNAME, name.c_str(),
           static_cast<int>(name.size()) + 1);

  struct rtattr *linkinfo = begin_nest(&req.n, IFLA_LINKINFO);
  add_attr(&req.n, IFLA_INFO_KIND, "bonding", 8);
  end_nest(&req.n, linkinfo);

  send(sock, &req, req.n.nlmsg_len, 0);
  close(sock);
}

void SystemConfigurationManager::SaveLagg(
    const LaggInterfaceConfig &lac) const {
  if (lac.name.empty())
    return;
  if (!InterfaceExists(lac.name))
    CreateLagg(lac.name);

  int sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
  if (sock < 0)
    return;

  int master_index = static_cast<int>(if_nametoindex(lac.name.c_str()));
  if (master_index == 0) {
    close(sock);
    return;
  }

  // Set Bond Mode (Protocol)
  struct nl_msg req{};
  req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
  req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
  req.n.nlmsg_type = RTM_NEWLINK;
  req.i.ifi_family = AF_UNSPEC;
  req.i.ifi_index = master_index;

  struct rtattr *linkinfo = begin_nest(&req.n, IFLA_LINKINFO);
  add_attr(&req.n, IFLA_INFO_KIND, "bonding", 8);
  struct rtattr *data = begin_nest(&req.n, IFLA_INFO_DATA);
  uint8_t mode = static_cast<uint8_t>(laggProtocolToBondMode(lac.protocol));
  add_attr(&req.n, IFLA_BOND_MODE, &mode, sizeof(mode));
  end_nest(&req.n, data);
  end_nest(&req.n, linkinfo);

  send(sock, &req, req.n.nlmsg_len, 0);

  // Add members
  for (const auto &member : lac.members) {
    int slave_index = static_cast<int>(if_nametoindex(member.c_str()));
    if (slave_index == 0)
      continue;

    std::memset(&req, 0, sizeof(req));
    req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
    req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
    req.n.nlmsg_type = RTM_NEWLINK;
    req.i.ifi_family = AF_UNSPEC;
    req.i.ifi_index = slave_index;
    add_attr(&req.n, IFLA_MASTER, &master_index, sizeof(master_index));
    send(sock, &req, req.n.nlmsg_len, 0);
  }

  close(sock);
}
