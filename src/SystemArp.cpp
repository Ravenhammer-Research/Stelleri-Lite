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

#include "ArpConfig.hpp"
#include "SystemConfigurationManager.hpp"

#include <arpa/inet.h>
#include <cstring>
#include <net/ethernet.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/route.h>
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

std::vector<ArpConfig> SystemConfigurationManager::GetArpEntries(
    const std::optional<std::string> &ip_filter,
    const std::optional<std::string> &iface_filter) const {
  std::vector<ArpConfig> entries;

  int mib[6];
  mib[0] = CTL_NET;
  mib[1] = PF_ROUTE;
  mib[2] = 0;
  mib[3] = AF_INET;
  mib[4] = NET_RT_FLAGS;
#ifdef RTF_LLINFO
  mib[5] = RTF_LLINFO;
#else
  mib[5] = 0;
#endif

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

    struct sockaddr_in *sin = reinterpret_cast<struct sockaddr_in *>(rtm + 1);
    struct sockaddr_dl *sdl = reinterpret_cast<struct sockaddr_dl *>(
        reinterpret_cast<char *>(sin) +
        ((sin->sin_len + sizeof(long) - 1) & ~(sizeof(long) - 1)));

    if (sin->sin_family != AF_INET || sdl->sdl_family != AF_LINK)
      continue;

    char ip_buf[INET_ADDRSTRLEN];
    if (!inet_ntop(AF_INET, &sin->sin_addr, ip_buf, sizeof(ip_buf)))
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

    ArpConfig entry;
    entry.ip = ip_buf;
    entry.iface = ifname;

    // Get MAC address
    if (sdl->sdl_alen == ETHER_ADDR_LEN) {
      auto *ea = reinterpret_cast<struct ether_addr *>(LLADDR(sdl));
      entry.mac = formatMAC(ea);
    } else {
      entry.mac = "(incomplete)";
    }

    // Check if permanent (no expiration)
    if (rtm->rtm_rmx.rmx_expire == 0) {
      entry.permanent = true;
    } else {
      struct timespec tp;
      clock_gettime(CLOCK_MONOTONIC, &tp);
      int expire_in = rtm->rtm_rmx.rmx_expire - tp.tv_sec;
      if (expire_in > 0)
        entry.expire = expire_in;
    }

    // Check if published (proxy ARP)
    if (rtm->rtm_flags & RTF_ANNOUNCE)
      entry.published = true;

    entries.push_back(std::move(entry));
  }

  return entries;
}

bool SystemConfigurationManager::SetArpEntry(
    const std::string &ip, const std::string &mac,
    const std::optional<std::string> &iface, bool temp, bool pub) const {
  // TODO: Implement ARP entry setting using routing socket (RTM_ADD)
  // Similar to the set() function in arp.c
  (void)ip;
  (void)mac;
  (void)iface;
  (void)temp;
  (void)pub;
  return false;
}

bool SystemConfigurationManager::DeleteArpEntry(
    const std::string &ip, const std::optional<std::string> &iface) const {
  // TODO: Implement ARP entry deletion using routing socket (RTM_DELETE)
  // Similar to the delete_rtsock() function in arp.c
  (void)ip;
  (void)iface;
  return false;
}
