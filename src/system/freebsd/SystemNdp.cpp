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

#include "NdpConfig.hpp"
#include "SystemConfigurationManager.hpp"

#include <arpa/inet.h>
#include <cstring>
#include <net/ethernet.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/route.h>
#include <netinet/icmp6.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <unistd.h>

namespace {

  // Format a 6-byte Ethernet address as "xx:xx:xx:xx:xx:xx".
  std::string formatMAC(const struct ether_addr *ea) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%02x:%02x:%02x:%02x:%02x:%02x",
                  ea->octet[0], ea->octet[1], ea->octet[2], ea->octet[3],
                  ea->octet[4], ea->octet[5]);
    return buf;
  }

} // namespace

std::vector<NdpConfig> SystemConfigurationManager::GetNdpEntries(
    const std::optional<std::string> &ip_filter,
    const std::optional<std::string> &iface_filter) const {
  std::vector<NdpConfig> entries;

  int mib[6];
  mib[0] = CTL_NET;
  mib[1] = PF_ROUTE;
  mib[2] = 0;
  mib[3] = AF_INET6;
  mib[4] = NET_RT_FLAGS;
  mib[5] = 0;

  size_t needed = 0;
  if (sysctl(mib, 6, nullptr, &needed, nullptr, 0) < 0 || needed == 0)
    return entries;

  std::vector<char> buf(needed);
  if (sysctl(mib, 6, buf.data(), &needed, nullptr, 0) < 0)
    return entries;

  char *lim = buf.data() + needed;
  for (char *next = buf.data(); next < lim;) {
    struct rt_msghdr *rtm = reinterpret_cast<struct rt_msghdr *>(next);
    if (rtm->rtm_msglen == 0)
      break;

    next += rtm->rtm_msglen;

    struct sockaddr_in6 *sin6 =
        reinterpret_cast<struct sockaddr_in6 *>(rtm + 1);
    struct sockaddr_dl *sdl = reinterpret_cast<struct sockaddr_dl *>(
        reinterpret_cast<char *>(sin6) +
        ((sin6->sin6_len + sizeof(long) - 1) & ~(sizeof(long) - 1)));

    if (sin6->sin6_family != AF_INET6 || sdl->sdl_family != AF_LINK)
      continue;

    // Skip if not a host route
    if (!(rtm->rtm_flags & RTF_HOST))
      continue;

    // Skip gateway entries (not NDP entries)
    if (rtm->rtm_flags & RTF_GATEWAY)
      continue;

    char ip_buf[INET6_ADDRSTRLEN];
    if (!inet_ntop(AF_INET6, &sin6->sin6_addr, ip_buf, sizeof(ip_buf)))
      continue;

    // Apply IP filter if specified
    if (ip_filter && *ip_filter != ip_buf)
      continue;

    // Get interface name
    char ifname[IF_NAMESIZE] = {0};
    if (if_indextoname(sdl->sdl_index, ifname) == nullptr)
      continue;

    // Apply interface filter if specified
    if (iface_filter && *iface_filter != ifname)
      continue;

    NdpConfig entry;
    entry.ip = ip_buf;
    entry.iface = ifname;

    // Get MAC address and link-layer metadata
    entry.ifindex = static_cast<int>(sdl->sdl_index);
    entry.sdl_alen = static_cast<int>(sdl->sdl_alen);
    if (sdl->sdl_alen == ETHER_ADDR_LEN) {
      auto *ea = reinterpret_cast<struct ether_addr *>(LLADDR(sdl));
      entry.mac = formatMAC(ea);
      entry.has_lladdr = true;
    } else {
      entry.mac = "(incomplete)";
      entry.has_lladdr = false;
    }

    // Copy rtm_rmx metrics and compute expiration if present
    entry.rmx_expire = rtm->rtm_rmx.rmx_expire;
    entry.rmx_mtu = rtm->rtm_rmx.rmx_mtu;
    entry.rmx_hopcount = rtm->rtm_rmx.rmx_hopcount;
    entry.rmx_rtt = rtm->rtm_rmx.rmx_rtt;
    entry.rmx_rttvar = rtm->rtm_rmx.rmx_rttvar;
    entry.rmx_recvpipe = rtm->rtm_rmx.rmx_recvpipe;
    entry.rmx_sendpipe = rtm->rtm_rmx.rmx_sendpipe;
    entry.rmx_ssthresh = rtm->rtm_rmx.rmx_ssthresh;
    entry.rmx_pksent = rtm->rtm_rmx.rmx_pksent;
    entry.rmx_weight = rtm->rtm_rmx.rmx_state;

    // Check if permanent (no expiration)
    if (entry.rmx_expire == 0) {
      entry.permanent = true;
    } else {
      struct timespec tp;
      clock_gettime(CLOCK_MONOTONIC, &tp);
      int expire_in = entry.rmx_expire - tp.tv_sec;
      if (expire_in > 0)
        entry.expire = expire_in;
    }

    // Copy raw flags and provenance
    entry.flags = rtm->rtm_flags;
    entry.rtm_type = rtm->rtm_type;
    entry.rtm_pid = rtm->rtm_pid;
    entry.rtm_seq = rtm->rtm_seq;
    entry.rtm_msglen = rtm->rtm_msglen;

    // Mark proxy/announce entries (RTF_ANNOUNCE) similar to ndp.c
    entry.is_proxy = (rtm->rtm_flags & RTF_ANNOUNCE) != 0;

    // Check if router
    if (rtm->rtm_flags & RTF_GATEWAY)
      entry.router = true;

    entries.push_back(std::move(entry));
  }

  return entries;
}

bool SystemConfigurationManager::SetNdpEntry(
    const std::string &ip, const std::string &mac,
    const std::optional<std::string> &iface, bool temp) const {
  // Build RTM_ADD routing socket message with RTA_DST and RTA_GATEWAY.
  // This mirrors the approach used by ndp(8): destination is the IPv6
  // neighbor address, gateway is a sockaddr_dl containing the link-layer
  // address. If `iface` is provided, use its index for the sockaddr_dl.

  auto roundup = [](size_t a) -> size_t {
    const size_t l = sizeof(long);
    return (a + l - 1) & ~(l - 1);
  };

  struct sockaddr_in6 sin6 = {};
  sin6.sin6_len = sizeof(sin6);
  sin6.sin6_family = AF_INET6;
  if (inet_pton(AF_INET6, ip.c_str(), &sin6.sin6_addr) != 1)
    return false;

  // Parse MAC string "aa:bb:cc:dd:ee:ff"
  unsigned char mac_bytes[8] = {};
  int mac_len = 0;
  {
    unsigned int b[8];
    char *s = const_cast<char *>(mac.c_str());
    int n = std::sscanf(s, "%x:%x:%x:%x:%x:%x", &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]);
    if (n < 6)
      return false;
    for (int i = 0; i < 6; ++i)
      mac_bytes[i] = static_cast<unsigned char>(b[i]);
    mac_len = 6;
  }

  // Prepare sockaddr_dl for gateway
  int ifindex = 0;
  if (iface) {
    ifindex = if_nametoindex(iface->c_str());
    if (ifindex == 0)
      return false;
  }

  // Create buffer for message
  std::vector<char> buf(512);
  char *p = buf.data();

  struct rt_msghdr *rtm = reinterpret_cast<struct rt_msghdr *>(p);
  std::memset(rtm, 0, sizeof(*rtm));
  p += sizeof(*rtm);

  // Copy destination sockaddr_in6
  std::memcpy(p, &sin6, sizeof(sin6));
  size_t sin6_sz = roundup(sizeof(sin6));
  p += sin6_sz;

  // Build sockaddr_dl. We'll allocate minimal sockaddr_dl and copy MAC into LLADDR.
  struct sockaddr_dl sdl = {};
  sdl.sdl_len = static_cast<unsigned char>(sizeof(struct sockaddr_dl) + mac_len);
  sdl.sdl_family = AF_LINK;
  sdl.sdl_index = ifindex;
  sdl.sdl_alen = static_cast<unsigned char>(mac_len);
  // sdl.sdl_data may contain name length; leave as 0
  // Copy sdl header, then write MAC into LLADDR(sdl)
  std::memcpy(p, &sdl, sizeof(struct sockaddr_dl));
  // LLADDR is pointer arithmetic on sockaddr_dl; compute offset
  char *ll = p + offsetof(struct sockaddr_dl, sdl_data);
  // For portability, ensure we don't overflow
  std::memcpy(ll, mac_bytes, mac_len);
  size_t sdl_sz = roundup(static_cast<size_t>(sdl.sdl_len));
  p += sdl_sz;

  rtm->rtm_msglen = static_cast<int>(p - buf.data());
  rtm->rtm_version = RTM_VERSION;
  rtm->rtm_type = RTM_ADD;
  rtm->rtm_flags = RTF_HOST | RTF_STATIC;
  rtm->rtm_addrs = RTA_DST | RTA_GATEWAY;

  // Send via routing socket
  #include "RoutingSocket.hpp"
  (void)temp;
  return WriteRoutingSocket(std::vector<char>(buf.data(), buf.data() + rtm->rtm_msglen), AF_INET6);
}

bool SystemConfigurationManager::DeleteNdpEntry(
    const std::string &ip, const std::optional<std::string> &iface) const {
  auto roundup = [](size_t a) -> size_t {
    const size_t l = sizeof(long);
    return (a + l - 1) & ~(l - 1);
  };

  struct sockaddr_in6 sin6 = {};
  sin6.sin6_len = sizeof(sin6);
  sin6.sin6_family = AF_INET6;
  if (inet_pton(AF_INET6, ip.c_str(), &sin6.sin6_addr) != 1)
    return false;

  std::vector<char> buf(256);
  char *p = buf.data();
  struct rt_msghdr *rtm = reinterpret_cast<struct rt_msghdr *>(p);
  std::memset(rtm, 0, sizeof(*rtm));
  p += sizeof(*rtm);

  std::memcpy(p, &sin6, sizeof(sin6));
  p += roundup(sizeof(sin6));

  rtm->rtm_msglen = static_cast<int>(p - buf.data());
  rtm->rtm_version = RTM_VERSION;
  rtm->rtm_type = RTM_DELETE;
  rtm->rtm_flags = RTF_HOST;
  rtm->rtm_addrs = RTA_DST;

  int s = socket(PF_ROUTE, SOCK_RAW, AF_INET6);
  if (s < 0)
    return false;
  (void)iface;
  ssize_t written = write(s, buf.data(), rtm->rtm_msglen);
  close(s);
  return (written == rtm->rtm_msglen);
}
