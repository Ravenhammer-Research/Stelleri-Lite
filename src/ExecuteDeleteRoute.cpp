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
#include "Parser.hpp"
#include "RouteConfig.hpp"
#include "RouteToken.hpp"
#include <iostream>

#include <arpa/inet.h>
#include <cstring>
#include <errno.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/route.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

void netcli::Parser::executeDeleteRoute(const RouteToken &tok,
                                        ConfigurationManager *mgr) const {
  (void)mgr;
  RouteConfig rc;
  rc.prefix = tok.prefix();
  if (tok.nexthop)
    rc.nexthop = tok.nexthop->toString();
  if (tok.interface)
    rc.iface = tok.interface->name();
  if (tok.vrf)
    rc.vrf = tok.vrf->name();
  rc.blackhole = tok.blackhole;
  rc.reject = tok.reject;

  auto net = IPNetwork::fromString(rc.prefix);
  if (!net) {
    std::cout << "delete route: invalid prefix: " << rc.prefix << "\n";
    return;
  }

  int s = socket(PF_ROUTE, SOCK_RAW, 0);
  if (s < 0) {
    std::cout << "delete route: open routing socket failed: " << strerror(errno)
              << "\n";
    return;
  }

  if (rc.vrf) {
    std::string v = *rc.vrf;
    int fib = -1;
    if (v.rfind("fib", 0) == 0) {
      try {
        fib = std::stoi(v.substr(3));
      } catch (...) {
        fib = -1;
      }
    } else {
      try {
        fib = std::stoi(v);
      } catch (...) {
        fib = -1;
      }
    }
    if (fib >= 0) {
      setsockopt(s, SOL_SOCKET, SO_SETFIB, &fib, sizeof(fib));
    }
  }

  struct {
    struct rt_msghdr m_rtm;
    char m_space[512];
  } m_rtmsg;
  memset(&m_rtmsg, 0, sizeof(m_rtmsg));

  struct rt_msghdr *rtm = &m_rtmsg.m_rtm;
  char *cp = m_rtmsg.m_space;

  rtm->rtm_version = RTM_VERSION;
  rtm->rtm_type = RTM_DELETE;
  rtm->rtm_seq = 1;
  rtm->rtm_pid = getpid();

  rtm->rtm_flags = RTF_UP | RTF_STATIC;
  if (rc.blackhole)
    rtm->rtm_flags |= RTF_BLACKHOLE;
  if (rc.reject)
    rtm->rtm_flags |= RTF_REJECT;

  // DST
  if (net->family() == AddressFamily::IPv4) {
    rtm->rtm_addrs |= RTA_DST;
    struct sockaddr_in sin_dst{};
    sin_dst.sin_len = sizeof(sin_dst);
    sin_dst.sin_family = AF_INET;
    std::string dst = net->address()->toString();
    inet_pton(AF_INET, dst.c_str(), &sin_dst.sin_addr);
    memcpy(cp, &sin_dst, sizeof(sin_dst));
    cp += sizeof(sin_dst);

    if (rc.nexthop) {
      rtm->rtm_addrs |= RTA_GATEWAY;
      struct sockaddr_in sin_gw{};
      sin_gw.sin_len = sizeof(sin_gw);
      sin_gw.sin_family = AF_INET;
      inet_pton(AF_INET, rc.nexthop->c_str(), &sin_gw.sin_addr);
      rtm->rtm_flags |= RTF_GATEWAY;
      memcpy(cp, &sin_gw, sizeof(sin_gw));
      cp += sizeof(sin_gw);
    }

    if (net->mask() < 32) {
      rtm->rtm_addrs |= RTA_NETMASK;
      struct sockaddr_in sin_mask{};
      sin_mask.sin_len = sizeof(sin_mask);
      sin_mask.sin_family = AF_INET;
      auto m = IPAddress::maskFromCIDR(AddressFamily::IPv4, net->mask());
      if (m) {
        inet_pton(AF_INET, m->toString().c_str(), &sin_mask.sin_addr);
      }
      memcpy(cp, &sin_mask, sizeof(sin_mask));
      cp += sizeof(sin_mask);
    }
  } else if (net->family() == AddressFamily::IPv6) {
    rtm->rtm_addrs |= RTA_DST;
    struct sockaddr_in6 sin6_dst{};
    sin6_dst.sin6_len = sizeof(sin6_dst);
    sin6_dst.sin6_family = AF_INET6;
    std::string dst = net->address()->toString();
    inet_pton(AF_INET6, dst.c_str(), &sin6_dst.sin6_addr);
    memcpy(cp, &sin6_dst, sizeof(sin6_dst));
    cp += sizeof(sin6_dst);

    if (rc.nexthop) {
      rtm->rtm_addrs |= RTA_GATEWAY;
      struct sockaddr_in6 sin6_gw{};
      sin6_gw.sin6_len = sizeof(sin6_gw);
      sin6_gw.sin6_family = AF_INET6;
      inet_pton(AF_INET6, rc.nexthop->c_str(), &sin6_gw.sin6_addr);
      rtm->rtm_flags |= RTF_GATEWAY;
      memcpy(cp, &sin6_gw, sizeof(sin6_gw));
      cp += sizeof(sin6_gw);
    }

    if (net->mask() < 128) {
      rtm->rtm_addrs |= RTA_NETMASK;
      struct sockaddr_in6 sin6_mask{};
      sin6_mask.sin6_len = sizeof(sin6_mask);
      sin6_mask.sin6_family = AF_INET6;
      auto m6 = IPAddress::maskFromCIDR(AddressFamily::IPv6, net->mask());
      if (m6) {
        inet_pton(AF_INET6, m6->toString().c_str(), &sin6_mask.sin6_addr);
      }
      memcpy(cp, &sin6_mask, sizeof(sin6_mask));
      cp += sizeof(sin6_mask);
    }
  }

  rtm->rtm_msglen =
      static_cast<u_short>(cp - reinterpret_cast<char *>(&m_rtmsg));

  ssize_t w = write(s, &m_rtmsg, rtm->rtm_msglen);
  if (w == -1) {
    std::cout << "delete route: write to routing socket failed: "
              << strerror(errno) << "\n";
    close(s);
    return;
  }

  std::cout << "delete route: " << rc.prefix << " removed\n";
  close(s);
}
