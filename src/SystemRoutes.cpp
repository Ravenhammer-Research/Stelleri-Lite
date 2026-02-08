/*
 * System route deletion implementation (routing socket)
 */

#include "SystemConfigurationManager.hpp"
#include "RouteConfig.hpp"
#include "IPAddress.hpp"
#include "Socket.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/route.h>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

void SystemConfigurationManager::DeleteRoute(const RouteConfig &rc) const {
  auto net = IPNetwork::fromString(rc.prefix);
  if (!net) {
    throw std::runtime_error("Invalid route prefix: " + rc.prefix);
  }

  Socket s(PF_ROUTE, SOCK_RAW);

  if (rc.vrf) {
    int fib = *rc.vrf;
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

  rtm->rtm_msglen = static_cast<u_short>(cp - reinterpret_cast<char *>(&m_rtmsg));

  ssize_t w = write(s, &m_rtmsg, rtm->rtm_msglen);
  if (w == -1) {
    throw std::runtime_error(std::string("write to routing socket failed: ") + strerror(errno));
  }
}

void SystemConfigurationManager::AddRoute(const RouteConfig &rc) const {
  auto net = IPNetwork::fromString(rc.prefix);
  if (!net) {
    throw std::runtime_error("Invalid route prefix: " + rc.prefix);
  }

  Socket s(PF_ROUTE, SOCK_RAW);

  if (rc.vrf) {
    int fib = *rc.vrf;
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
  rtm->rtm_type = RTM_ADD;
  rtm->rtm_seq = 1;
  rtm->rtm_pid = getpid();

  rtm->rtm_flags = RTF_UP | RTF_STATIC;
  if (rc.blackhole)
    rtm->rtm_flags |= RTF_BLACKHOLE;
  if (rc.reject)
    rtm->rtm_flags |= RTF_REJECT;

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

  rtm->rtm_msglen = static_cast<u_short>(cp - reinterpret_cast<char *>(&m_rtmsg));

  ssize_t w = write(s, &m_rtmsg, rtm->rtm_msglen);
  if (w == -1) {
    throw std::runtime_error(std::string("write to routing socket failed: ") + strerror(errno));
  }
}
