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

#include "Socket.hpp"
#include "SystemConfigurationManager.hpp"
#include "VRFConfig.hpp"
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

  int get_vrf_table(int sock, int ifindex) {
    struct {
      struct nlmsghdr n;
      struct ifinfomsg i;
    } req{};

    req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
    req.n.nlmsg_flags = NLM_F_REQUEST;
    req.n.nlmsg_type = RTM_GETLINK;
    req.i.ifi_family = AF_UNSPEC;
    req.i.ifi_index = ifindex;

    if (send(sock, &req, req.n.nlmsg_len, 0) < 0)
      return 0;

    char buf[4096];
    ssize_t len = recv(sock, buf, sizeof(buf), 0);
    if (len <= 0)
      return 0;

    struct nlmsghdr *nh = (struct nlmsghdr *)buf;
    for (; NLMSG_OK(nh, static_cast<uint32_t>(len)); nh = NLMSG_NEXT(nh, len)) {
      if (nh->nlmsg_type == NLMSG_ERROR)
        return 0;
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
          struct rtattr *info_data = nullptr;

          for (; RTA_OK(li, li_len); li = RTA_NEXT(li, li_len)) {
            if (li->rta_type == IFLA_INFO_DATA) {
              info_data = li;
              break;
            }
          }

          if (info_data) {
            struct rtattr *id = (struct rtattr *)RTA_DATA(info_data);
            int id_len = static_cast<int>(RTA_PAYLOAD(info_data));
            for (; RTA_OK(id, id_len); id = RTA_NEXT(id, id_len)) {
              if (id->rta_type == IFLA_VRF_TABLE) {
                return *(int *)RTA_DATA(id);
              }
            }
          }
        }
      }
    }
    return 0;
  }

} // namespace

void SystemConfigurationManager::CreateVrf(const VRFConfig &vrf) const {
  if (vrf.name.empty())
    return;

  int sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
  if (sock < 0)
    return;

  struct nl_msg req{};
  req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
  req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL | NLM_F_ACK;
  req.n.nlmsg_type = RTM_NEWLINK;
  req.i.ifi_family = AF_UNSPEC;

  add_attr(&req.n, IFLA_IFNAME, vrf.name.c_str(),
           static_cast<int>(vrf.name.size()) + 1);

  struct rtattr *linkinfo = begin_nest(&req.n, IFLA_LINKINFO);
  add_attr(&req.n, IFLA_INFO_KIND, "vrf", 4);
  struct rtattr *data = begin_nest(&req.n, IFLA_INFO_DATA);
  uint32_t table = static_cast<uint32_t>(vrf.table);
  add_attr(&req.n, IFLA_VRF_TABLE, &table, sizeof(table));
  end_nest(&req.n, data);
  end_nest(&req.n, linkinfo);

  send(sock, &req, req.n.nlmsg_len, 0);

  // Bring it UP
  int ifindex = if_nametoindex(vrf.name.c_str());
  if (ifindex != 0) {
    std::memset(&req, 0, sizeof(req));
    req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
    req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
    req.n.nlmsg_type = RTM_NEWLINK;
    req.i.ifi_family = AF_UNSPEC;
    req.i.ifi_index = ifindex;
    req.i.ifi_flags = IFF_UP;
    req.i.ifi_change = IFF_UP;
    send(sock, &req, req.n.nlmsg_len, 0);
  }

  close(sock);
}

void SystemConfigurationManager::DeleteVrf(const std::string &name) const {
  if (name.empty())
    return;

  int sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
  if (sock < 0)
    return;

  struct nl_msg req{};
  req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
  req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
  req.n.nlmsg_type = RTM_DELLINK;
  req.i.ifi_family = AF_UNSPEC;

  add_attr(&req.n, IFLA_IFNAME, name.c_str(),
           static_cast<int>(name.size()) + 1);

  send(sock, &req, req.n.nlmsg_len, 0);
  close(sock);
}

std::vector<VRFConfig> SystemConfigurationManager::GetVrfs() const {
  std::vector<VRFConfig> results;
  int sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
  if (sock < 0)
    return results;

  auto interfaces = GetInterfaces();
  for (const auto &ic : interfaces) {
    if (ic.type == InterfaceType::VRF) {
      int table = get_vrf_table(sock, ic.index.value_or(0));
      results.emplace_back(ic.name, table);
    }
  }
  close(sock);
  return results;
}
