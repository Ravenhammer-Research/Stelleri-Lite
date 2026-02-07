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

#include "InterfaceConfig.hpp"
#include "SystemConfigurationManager.hpp"
#include "WlanAuthMode.hpp"
#include "WlanConfig.hpp"

#include <cstring>
#include <ifaddrs.h>
#include <net/if.h>
#include <net80211/ieee80211_ioctl.h>
#include <netinet/in.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <unistd.h>
#include <unordered_map>

void SystemConfigurationManager::prepare_ifreq(struct ifreq &ifr,
                                               const std::string &name) const {
  std::memset(&ifr, 0, sizeof(ifr));
  std::strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ - 1);
}

std::optional<int> SystemConfigurationManager::query_ifreq_int(
    const std::string &ifname, unsigned long req, IfreqIntField which) const {
  int s = socket(AF_INET, SOCK_DGRAM, 0);
  if (s < 0)
    return std::nullopt;
  struct ifreq ifr;
  prepare_ifreq(ifr, ifname);
  if (ioctl(s, req, &ifr) == 0) {
    int v = 0;
    switch (which) {
    case IfreqIntField::Metric:
      v = ifr.ifr_metric;
      break;
    case IfreqIntField::Fib:
      v = ifr.ifr_fib;
      break;
    case IfreqIntField::Mtu:
      v = ifr.ifr_mtu;
      break;
    }
    close(s);
    return v;
  }
  close(s);
  return std::nullopt;
}

std::optional<std::pair<std::string, int>>
SystemConfigurationManager::query_ifreq_sockaddr(const std::string &ifname,
                                                 unsigned long req) const {
  int s = socket(AF_INET, SOCK_DGRAM, 0);
  if (s < 0)
    return std::nullopt;
  struct ifreq ifr;
  prepare_ifreq(ifr, ifname);
  if (ioctl(s, req, &ifr) == 0) {
    if (ifr.ifr_addr.sa_family == AF_INET) {
      struct sockaddr_in *sin =
          reinterpret_cast<struct sockaddr_in *>(&ifr.ifr_addr);
      char buf[INET_ADDRSTRLEN] = {0};
      if (inet_ntop(AF_INET, &sin->sin_addr, buf, sizeof(buf))) {
        close(s);
        return std::make_pair(std::string(buf), AF_INET);
      }
    } else if (ifr.ifr_addr.sa_family == AF_INET6) {
      struct sockaddr_in6 *sin6 =
          reinterpret_cast<struct sockaddr_in6 *>(&ifr.ifr_addr);
      char buf[INET6_ADDRSTRLEN] = {0};
      if (inet_ntop(AF_INET6, &sin6->sin6_addr, buf, sizeof(buf))) {
        close(s);
        return std::make_pair(std::string(buf), AF_INET6);
      }
    }
  }
  close(s);
  return std::nullopt;
}

std::vector<std::string> SystemConfigurationManager::query_interface_groups(
    const std::string &ifname) const {
  std::vector<std::string> out;
  int s = socket(AF_LOCAL, SOCK_DGRAM, 0);
  if (s < 0)
    return out;
  struct ifgroupreq ifgr;
  std::memset(&ifgr, 0, sizeof(ifgr));
  std::strncpy(ifgr.ifgr_name, ifname.c_str(), IFNAMSIZ - 1);
  if (ioctl(s, SIOCGIFGROUP, &ifgr) == 0) {
    size_t len = ifgr.ifgr_len;
    if (len > 0) {
      size_t count = len / sizeof(struct ifg_req);
      std::vector<struct ifg_req> groups(count);
      ifgr.ifgr_groups = groups.data();
      if (ioctl(s, SIOCGIFGROUP, &ifgr) == 0) {
        for (size_t i = 0; i < count; ++i) {
          std::string gname(groups[i].ifgrq_group);
          if (!gname.empty())
            out.emplace_back(gname);
        }
      }
    }
  }
  close(s);
  return out;
}

std::optional<std::string> SystemConfigurationManager::query_ifstatus_nd6(
    const std::string &ifname) const {
  (void)ifname;
  return std::nullopt;
}

void SystemConfigurationManager::populateInterfaceMetadata(
    InterfaceConfig &ic) const {
  if (auto m = query_ifreq_int(ic.name, SIOCGIFMETRIC, IfreqIntField::Metric);
      m) {
    ic.metric = *m;
  }
  if (auto f = query_ifreq_int(ic.name, SIOCGIFFIB, IfreqIntField::Fib); f) {
    if (!ic.vrf)
      ic.vrf = std::make_unique<VRFConfig>(*f);
    else
      ic.vrf->table = *f;
  }
  unsigned int ifidx = if_nametoindex(ic.name.c_str());
  if (ifidx != 0) {
    ic.index = static_cast<int>(ifidx);
  }

  auto groups = query_interface_groups(ic.name);
  for (const auto &g : groups)
    ic.groups.emplace_back(g);
  if (auto nd6 = query_ifstatus_nd6(ic.name); nd6)
    ic.nd6_options = *nd6;

  if (ic.type == InterfaceType::Wireless) {
    if (auto w = dynamic_cast<WlanConfig *>(&ic)) {
      int s = socket(AF_INET, SOCK_DGRAM, 0);
      if (s >= 0) {
        struct ieee80211req req;
        std::memset(&req, 0, sizeof(req));
        std::strncpy(req.i_name, ic.name.c_str(), IFNAMSIZ - 1);

        {
          const int bufsize = 256;
          std::vector<char> buf(bufsize);
          req.i_type = IEEE80211_IOC_SSID;
          req.i_len = bufsize;
          req.i_data = buf.data();
          if (ioctl(s, SIOCG80211, &req) == 0 && req.i_len > 0) {
            w->ssid = std::string(buf.data(), static_cast<size_t>(req.i_len));
          }
        }

        {
          int ch = 0;
          req.i_type = IEEE80211_IOC_CURCHAN;
          req.i_data = &ch;
          req.i_len = sizeof(ch);
          if (ioctl(s, SIOCG80211, &req) == 0) {
            w->channel = ch;
          } else {
            ch = 0;
            req.i_type = IEEE80211_IOC_CHANNEL;
            req.i_data = &ch;
            req.i_len = sizeof(ch);
            if (ioctl(s, SIOCG80211, &req) == 0)
              w->channel = ch;
          }
        }

        {
          const int macsz = 8;
          std::vector<unsigned char> mac(macsz);
          req.i_type = IEEE80211_IOC_BSSID;
          req.i_len = macsz;
          req.i_data = mac.data();
          if (ioctl(s, SIOCG80211, &req) == 0 && req.i_len >= 6) {
            char macbuf[32];
            std::snprintf(macbuf, sizeof(macbuf),
                          "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1],
                          mac[2], mac[3], mac[4], mac[5]);
            w->bssid = std::string(macbuf);
          }
        }

        {
          const int psize = 64;
          std::vector<char> pbuf(psize);
          req.i_type = IEEE80211_IOC_IC_NAME;
          req.i_len = psize;
          req.i_data = pbuf.data();
          if (ioctl(s, SIOCG80211, &req) == 0 && req.i_len > 0)
            w->parent =
                std::string(pbuf.data(), static_cast<size_t>(req.i_len));
        }

        {
          int am = 0;
          req.i_type = IEEE80211_IOC_AUTHMODE;
          req.i_data = &am;
          req.i_len = sizeof(am);
          if (ioctl(s, SIOCG80211, &req) == 0) {
            WlanAuthMode m = WlanAuthMode::UNKNOWN;
            switch (am) {
            case 0:
              m = WlanAuthMode::OPEN;
              break;
            case 1:
              m = WlanAuthMode::SHARED;
              break;
            case 2:
              m = WlanAuthMode::WPA;
              break;
            case 3:
              m = WlanAuthMode::WPA2;
              break;
            default:
              m = WlanAuthMode::UNKNOWN;
              break;
            }
            w->authmode = m;
          }
        }

        if (ic.flags && (*ic.flags & IFF_RUNNING)) {
          if (w->ssid)
            w->status = std::string("associated");
          else
            w->status = std::string("up");
        } else {
          w->status = std::string("down");
        }
        if (w->channel) {
          w->media =
              std::string("IEEE 802.11 channel ") + std::to_string(*w->channel);
        }

        close(s);
      }
    }
  }
}

std::vector<InterfaceConfig> SystemConfigurationManager::GetInterfaces(
    const std::optional<VRFConfig> &vrf) const {
  std::vector<InterfaceConfig> out;
  struct ifaddrs *ifs = nullptr;
  if (getifaddrs(&ifs) != 0)
    return out;

  std::unordered_map<std::string, std::shared_ptr<InterfaceConfig>> map;
  for (struct ifaddrs *ifa = ifs; ifa; ifa = ifa->ifa_next) {
    if (!ifa->ifa_name)
      continue;
    std::string name = ifa->ifa_name;
    auto it = map.find(name);
    if (it == map.end()) {
      auto ic = std::make_shared<InterfaceConfig>(ifa);
      if (ic->type == InterfaceType::Wireless) {
        auto w = std::make_shared<WlanConfig>(*ic);
        map.emplace(name, std::move(w));
      } else {
        map.emplace(name, std::move(ic));
      }
    } else {
      auto existing = it->second;
      if (ifa->ifa_addr) {
        if (ifa->ifa_addr->sa_family == AF_INET ||
            ifa->ifa_addr->sa_family == AF_INET6) {
          InterfaceConfig tmp(ifa);
          if (tmp.address) {
            if (!existing->address) {
              existing->address = std::move(tmp.address);
            } else {
              existing->aliases.emplace_back(std::move(tmp.address));
            }
          }
        }
      }
    }
  }

  for (auto &kv : map) {
    populateInterfaceMetadata(*kv.second);
    for (const auto &g : kv.second->groups) {
      if (g == "epair") {
        kv.second->type = InterfaceType::Virtual;
        break;
      }
    }
    if (interfaceIsLagg(kv.second->name)) {
      kv.second->type = InterfaceType::Lagg;
    } else if (interfaceIsBridge(kv.second->name)) {
      kv.second->type = InterfaceType::Bridge;
    }
    if (matches_vrf(*kv.second, vrf))
      out.emplace_back(std::move(*kv.second));
  }

  freeifaddrs(ifs);
  return out;
}

std::vector<InterfaceConfig> SystemConfigurationManager::GetInterfacesByGroup(
    const std::optional<VRFConfig> &vrf, std::string_view group) const {
  auto bases = GetInterfaces(vrf);
  std::vector<InterfaceConfig> out;
  for (const auto &ic : bases) {
    for (const auto &g : ic.groups) {
      if (g == group) {
        out.push_back(ic);
        break;
      }
    }
  }
  return out;
}
