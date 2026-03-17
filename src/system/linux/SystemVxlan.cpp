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
#include "VxlanInterfaceConfig.hpp"
#include <arpa/inet.h>
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

} // namespace

std::vector<VxlanInterfaceConfig>
SystemConfigurationManager::GetVxlanInterfaces(
    const std::vector<InterfaceConfig> &bases) const {
  std::vector<VxlanInterfaceConfig> results;
  int sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
  if (sock < 0)
    return results;

  for (const auto &base : bases) {
    if (base.type == InterfaceType::VXLAN) {
      VxlanInterfaceConfig vc(base);

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
                      if (id->rta_type == IFLA_VXLAN_ID) {
                        vc.vni = *(uint32_t *)RTA_DATA(id);
                      } else if (id->rta_type == IFLA_VXLAN_LOCAL) {
                        char abuf[INET_ADDRSTRLEN];
                        inet_ntop(AF_INET, RTA_DATA(id), abuf, sizeof(abuf));
                        vc.localAddr = abuf;
                      } else if (id->rta_type == IFLA_VXLAN_REMOTE) {
                        char abuf[INET_ADDRSTRLEN];
                        inet_ntop(AF_INET, RTA_DATA(id), abuf, sizeof(abuf));
                        vc.remoteAddr = abuf;
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
      results.push_back(std::move(vc));
    }
  }

  close(sock);
  return results;
}

void SystemConfigurationManager::CreateVxlan(const std::string &name) const {
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
  add_attr(&req.n, IFLA_INFO_KIND, "vxlan", 6);
  end_nest(&req.n, linkinfo);

  send(sock, &req, req.n.nlmsg_len, 0);
  close(sock);
}

void SystemConfigurationManager::SaveVxlan(
    const VxlanInterfaceConfig &vxlan) const {
  if (vxlan.name.empty())
    return;
  if (!InterfaceExists(vxlan.name))
    CreateVxlan(vxlan.name);

  int sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
  if (sock < 0)
    return;

  int ifindex = static_cast<int>(if_nametoindex(vxlan.name.c_str()));
  if (ifindex == 0) {
    close(sock);
    return;
  }

  struct nl_msg req{};
  req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
  req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
  req.n.nlmsg_type = RTM_NEWLINK;
  req.i.ifi_family = AF_UNSPEC;
  req.i.ifi_index = ifindex;

  struct rtattr *linkinfo = begin_nest(&req.n, IFLA_LINKINFO);
  add_attr(&req.n, IFLA_INFO_KIND, "vxlan", 6);
  struct rtattr *data = begin_nest(&req.n, IFLA_INFO_DATA);

  if (vxlan.vni) {
    uint32_t vni = *vxlan.vni;
    add_attr(&req.n, IFLA_VXLAN_ID, &vni, sizeof(vni));
  }
  if (vxlan.localAddr) {
    struct in_addr addr;
    if (inet_pton(AF_INET, vxlan.localAddr->c_str(), &addr) == 1) {
      add_attr(&req.n, IFLA_VXLAN_LOCAL, &addr, sizeof(addr));
    }
  }
  if (vxlan.remoteAddr) {
    struct in_addr addr;
    if (inet_pton(AF_INET, vxlan.remoteAddr->c_str(), &addr) == 1) {
      add_attr(&req.n, IFLA_VXLAN_REMOTE, &addr, sizeof(addr));
    }
  }
  if (vxlan.remotePort) {
    uint16_t port = htons(*vxlan.remotePort);
    add_attr(&req.n, IFLA_VXLAN_PORT, &port, sizeof(port));
  }

  end_nest(&req.n, data);
  end_nest(&req.n, linkinfo);

  send(sock, &req, req.n.nlmsg_len, 0);
  close(sock);
}
