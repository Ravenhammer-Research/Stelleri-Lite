/*
 * Copyright (c) 2026, Ravenhammer Research Inc.
 * All rights reserved.
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
#include <sstream>
#include <iomanip>
#include <arpa/inet.h>

static inline size_t roundup_sa_len(int len) {
  if (len <= 0)
    return sizeof(long);
  return 1 + (((len - 1) | (sizeof(long) - 1)));
}

std::vector<RouteConfig> SystemConfigurationManager::GetStaticRoutes(
    const std::optional<VRFConfig> &vrf) const {
  std::vector<RouteConfig> routes;
  int fibnum = 0;
  if (vrf)
    fibnum = vrf->table;
  int mib[7] = {CTL_NET, PF_ROUTE, 0, AF_UNSPEC, NET_RT_DUMP, 0, fibnum};
  size_t needed = 0;
  if (sysctl(mib, 7, nullptr, &needed, nullptr, 0) < 0 || needed == 0)
    return routes;

  std::vector<char> buf(needed);
  if (sysctl(mib, 7, buf.data(), &needed, nullptr, 0) < 0)
    return routes;

  char *lim = buf.data() + needed;
  for (char *next = buf.data(); next < lim;) {
    struct rt_msghdr *rtm = reinterpret_cast<struct rt_msghdr *>(next);
    if (rtm->rtm_msglen == 0)
      break;

    if (rtm->rtm_type != RTM_GET && rtm->rtm_type != RTM_ADD) {
      next += rtm->rtm_msglen;
      continue;
    }

    struct sockaddr *sa = reinterpret_cast<struct sockaddr *>(rtm + 1);
    struct sockaddr *rti_info[RouteConfig::RTAX(RouteConfig::RTAX::COUNT)];
    std::memset(rti_info, 0, sizeof(rti_info));

    for (int i = 0; i < RouteConfig::RTAX(RouteConfig::RTAX::COUNT) &&
            reinterpret_cast<char *>(sa) < next + rtm->rtm_msglen;
       i++) {
      if ((rtm->rtm_addrs & (1 << i)) == 0)
        continue;
      rti_info[i] = sa;
      sa = reinterpret_cast<struct sockaddr *>(reinterpret_cast<char *>(sa) +
                                               roundup_sa_len(sa->sa_len));
    }

    RouteConfig rc;

    if (vrf)
      rc.vrf = vrf->table;

    // Raw routing message provenance
    rc.rtm_type = rtm->rtm_type;
    rc.rtm_pid = rtm->rtm_pid;
    rc.rtm_seq = rtm->rtm_seq;
    rc.rtm_msglen = rtm->rtm_msglen;

    // copy metrics from rtm_rmx
    rc.rmx_mtu = rtm->rtm_rmx.rmx_mtu;
    rc.rmx_hopcount = rtm->rtm_rmx.rmx_hopcount;
    rc.rmx_rtt = rtm->rtm_rmx.rmx_rtt;
    rc.rmx_rttvar = rtm->rtm_rmx.rmx_rttvar;
    rc.rmx_recvpipe = rtm->rtm_rmx.rmx_recvpipe;
    rc.rmx_sendpipe = rtm->rtm_rmx.rmx_sendpipe;
    rc.rmx_ssthresh = rtm->rtm_rmx.rmx_ssthresh;
    rc.rmx_pksent = rtm->rtm_rmx.rmx_pksent;

    // capture IFA (interface address) when present
    if (rti_info[RouteConfig::RTAX(RouteConfig::RTAX::IFA)]) {
      struct sockaddr *saifa = rti_info[RouteConfig::RTAX(RouteConfig::RTAX::IFA)];
      char buf_ifa[INET6_ADDRSTRLEN] = {0};
      if (saifa->sa_family == AF_INET) {
        auto sin = reinterpret_cast<struct sockaddr_in *>(saifa);
        inet_ntop(AF_INET, &sin->sin_addr, buf_ifa, sizeof(buf_ifa));
        rc.ifa = std::string(buf_ifa);
      } else if (saifa->sa_family == AF_INET6) {
        auto sin6 = reinterpret_cast<struct sockaddr_in6 *>(saifa);
        inet_ntop(AF_INET6, &sin6->sin6_addr, buf_ifa, sizeof(buf_ifa));
        rc.ifa = std::string(buf_ifa);
      }
    }

    // capture IFP (interface info) when present (usually AF_LINK)
    if (rti_info[RouteConfig::RTAX(RouteConfig::RTAX::IFP)]) {
      struct sockaddr *saifp = rti_info[RouteConfig::RTAX(RouteConfig::RTAX::IFP)];
      if (saifp->sa_family == AF_LINK) {
        auto sdl = reinterpret_cast<struct sockaddr_dl *>(saifp);
        if (sdl->sdl_nlen > 0)
          rc.ifp = std::string(sdl->sdl_data, sdl->sdl_nlen);
      }
    }

    // capture AUTHOR and BRD if present
    if (rti_info[RouteConfig::RTAX(RouteConfig::RTAX::AUTHOR)]) {
      struct sockaddr *saa = rti_info[RouteConfig::RTAX(RouteConfig::RTAX::AUTHOR)];
      char buf_author[INET6_ADDRSTRLEN] = {0};
      if (saa->sa_family == AF_INET) {
        auto sin = reinterpret_cast<struct sockaddr_in *>(saa);
        inet_ntop(AF_INET, &sin->sin_addr, buf_author, sizeof(buf_author));
        rc.author = std::string(buf_author);
      } else if (saa->sa_family == AF_INET6) {
        auto sin6 = reinterpret_cast<struct sockaddr_in6 *>(saa);
        inet_ntop(AF_INET6, &sin6->sin6_addr, buf_author, sizeof(buf_author));
        rc.author = std::string(buf_author);
      }
    }
    if (rti_info[RouteConfig::RTAX(RouteConfig::RTAX::BRD)]) {
      struct sockaddr *sabr = rti_info[RouteConfig::RTAX(RouteConfig::RTAX::BRD)];
      char buf_brd[INET6_ADDRSTRLEN] = {0};
      if (sabr->sa_family == AF_INET) {
        auto sin = reinterpret_cast<struct sockaddr_in *>(sabr);
        inet_ntop(AF_INET, &sin->sin_addr, buf_brd, sizeof(buf_brd));
        rc.brd = std::string(buf_brd);
      } else if (sabr->sa_family == AF_INET6) {
        auto sin6 = reinterpret_cast<struct sockaddr_in6 *>(sabr);
        inet_ntop(AF_INET6, &sin6->sin6_addr, buf_brd, sizeof(buf_brd));
        rc.brd = std::string(buf_brd);
      }
    }

    if (rti_info[RouteConfig::RTAX(RouteConfig::RTAX::DST)]) {
      char buf_dst[INET6_ADDRSTRLEN] = {0};
      int prefixlen = 0;
      if (rti_info[RouteConfig::RTAX(RouteConfig::RTAX::DST)]->sa_family ==
          AF_INET) {
        auto sin = reinterpret_cast<struct sockaddr_in *>(
            rti_info[RouteConfig::RTAX(RouteConfig::RTAX::DST)]);
        inet_ntop(AF_INET, &sin->sin_addr, buf_dst, sizeof(buf_dst));
        if (rti_info[RouteConfig::RTAX(RouteConfig::RTAX::NETMASK)]) {
          auto mask = reinterpret_cast<struct sockaddr_in *>(
              rti_info[RouteConfig::RTAX(RouteConfig::RTAX::NETMASK)]);
          uint32_t m = ntohl(mask->sin_addr.s_addr);
          prefixlen = __builtin_popcount(m);
        } else
          prefixlen = 32;
        rc.prefix = std::string(buf_dst) + "/" + std::to_string(prefixlen);
        } else if (rti_info[RouteConfig::RTAX(RouteConfig::RTAX::DST)]
               ->sa_family == AF_INET6) {
        auto sin6 = reinterpret_cast<struct sockaddr_in6 *>(
          rti_info[RouteConfig::RTAX(RouteConfig::RTAX::DST)]);
        inet_ntop(AF_INET6, &sin6->sin6_addr, buf_dst, sizeof(buf_dst));
        if (rti_info[RouteConfig::RTAX(RouteConfig::RTAX::NETMASK)]) {
          auto mask6 = reinterpret_cast<struct sockaddr_in6 *>(
              rti_info[RouteConfig::RTAX(RouteConfig::RTAX::NETMASK)]);
          prefixlen = 0;
          for (int j = 0; j < 16; j++)
            prefixlen += __builtin_popcount(mask6->sin6_addr.s6_addr[j]);
        } else
          prefixlen = 128;
        if (sin6->sin6_scope_id != 0) {
          char ifn[IF_NAMESIZE] = {0};
          if (if_indextoname(static_cast<unsigned int>(sin6->sin6_scope_id),
                             ifn)) {
            rc.scope = std::string(ifn);
          }
        }
        rc.prefix = std::string(buf_dst) + "/" + std::to_string(prefixlen);
      }
    }

    if (rti_info[RouteConfig::RTAX(RouteConfig::RTAX::GATEWAY)]) {
      char buf_gw[INET6_ADDRSTRLEN] = {0};
      if (rti_info[RouteConfig::RTAX(RouteConfig::RTAX::GATEWAY)]->sa_family ==
          AF_INET) {
        auto sin = reinterpret_cast<struct sockaddr_in *>(
            rti_info[RouteConfig::RTAX(RouteConfig::RTAX::GATEWAY)]);
        if (inet_ntop(AF_INET, &sin->sin_addr, buf_gw, sizeof(buf_gw)))
          rc.nexthop = buf_gw;
      } else if (rti_info[RouteConfig::RTAX(RouteConfig::RTAX::GATEWAY)]
                     ->sa_family == AF_INET6) {
        auto sin6 = reinterpret_cast<struct sockaddr_in6 *>(
            rti_info[RouteConfig::RTAX(RouteConfig::RTAX::GATEWAY)]);
        if (inet_ntop(AF_INET6, &sin6->sin6_addr, buf_gw, sizeof(buf_gw)))
          rc.nexthop = buf_gw;
      } else if (rti_info[RouteConfig::RTAX(RouteConfig::RTAX::GATEWAY)]
                     ->sa_family == AF_LINK) {
        auto sdl = reinterpret_cast<struct sockaddr_dl *>(
            rti_info[RouteConfig::RTAX(RouteConfig::RTAX::GATEWAY)]);
        if (sdl->sdl_index > 0) {
          rc.nexthop = std::string("link#") + std::to_string(sdl->sdl_index);
        }
        if (sdl->sdl_nlen > 0)
          rc.iface = std::string(sdl->sdl_data, sdl->sdl_nlen);
        // Capture link-layer (MAC) address if available
        if (sdl->sdl_alen > 0) {
          unsigned char *mac = reinterpret_cast<unsigned char *>(sdl->sdl_data + sdl->sdl_nlen);
          std::ostringstream macs;
          macs << std::hex << std::setfill('0');
          for (int mi = 0; mi < sdl->sdl_alen; mi++) {
            if (mi)
              macs << ':';
            macs << std::setw(2) << static_cast<int>(mac[mi]);
          }
          rc.gateway_hw = macs.str();
        }
      }
    }

    if (rtm->rtm_index > 0) {
      rc.iface_index = static_cast<int>(rtm->rtm_index);
      char ifname[IF_NAMESIZE];
      if (if_indextoname(rtm->rtm_index, ifname)) {
        if (!rc.iface)
          rc.iface = ifname;
      }
    }

    if (rtm->rtm_flags & RouteConfig::Flag(RouteConfig::BLACKHOLE))
      rc.blackhole = true;
    if (rtm->rtm_flags & RouteConfig::Flag(RouteConfig::REJECT))
      rc.reject = true;

    rc.flags = static_cast<unsigned int>(rtm->rtm_flags);
    if (rtm->rtm_rmx.rmx_expire != 0)
      rc.expire = static_cast<int>(rtm->rtm_rmx.rmx_expire);

    

    if (!rc.prefix.empty()) {
      if (vrf) {
        if (!rc.vrf) {
        } else if (*rc.vrf != vrf->table) {
        } else
          routes.push_back(rc);
      } else {
        routes.push_back(rc);
      }
    }

    next += rtm->rtm_msglen;
  }

  return routes;
}
/*
 * System route deletion implementation (routing socket)
 */

#include "IPAddress.hpp"
#include "RouteConfig.hpp"
#include "Socket.hpp"
#include "SystemConfigurationManager.hpp"

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

// Shared helper for RTM_ADD / RTM_DELETE via routing socket.
static void routeSocketOp(const RouteConfig &rc, int rtm_type) {
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
  rtm->rtm_type = rtm_type;
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

  rtm->rtm_msglen =
      static_cast<u_short>(cp - reinterpret_cast<char *>(&m_rtmsg));

  ssize_t w = write(s, &m_rtmsg, rtm->rtm_msglen);
  if (w == -1) {
    throw std::runtime_error(std::string("write to routing socket failed: ") +
                             strerror(errno));
  }
}

void SystemConfigurationManager::DeleteRoute(const RouteConfig &rc) const {
  routeSocketOp(rc, RTM_DELETE);
}

void SystemConfigurationManager::AddRoute(const RouteConfig &rc) const {
  routeSocketOp(rc, RTM_ADD);
}

std::vector<RouteConfig> SystemConfigurationManager::GetRoutes(
    const std::optional<VRFConfig> &vrf) const {
  return GetStaticRoutes(vrf);
}
