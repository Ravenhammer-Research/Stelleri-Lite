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

#include "IPAddress.hpp"
#include "IPNetwork.hpp"
#include "RouteConfig.hpp"
#include "SystemConfigurationManager.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <linux/route.h>
#include <sstream>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {
  std::string hexToIp(const std::string &hex) {
    uint32_t ip;
    std::stringstream ss;
    ss << std::hex << hex;
    ss >> ip;
    struct in_addr addr;
    addr.s_addr = ip;
    return inet_ntoa(addr);
  }

  int countSetBits(uint32_t n) {
    int count = 0;
    while (n > 0) {
      n &= (n - 1);
      count++;
    }
    return count;
  }
} // namespace

std::vector<RouteConfig>
SystemConfigurationManager::GetStaticRoutes(const std::optional<VRFConfig> &vrf
                                            [[maybe_unused]]) const {
  std::vector<RouteConfig> routes;

  // IPv4 - Listing routes via ioctl is not standard on Linux, using
  // /proc/net/route for enumeration.
  std::ifstream ipv4_route("/proc/net/route");
  if (ipv4_route.is_open()) {
    std::string line;
    std::getline(ipv4_route, line); // Skip header
    while (std::getline(ipv4_route, line)) {
      std::stringstream ss(line);
      std::string iface, dest, gateway, flags, refcnt, use, metric, mask, mtu,
          window, irtt;
      ss >> iface >> dest >> gateway >> flags >> refcnt >> use >> metric >>
          mask >> mtu >> window >> irtt;

      RouteConfig rc;
      rc.iface = iface;

      std::string dest_ip = hexToIp(dest);
      uint32_t mask_val = 0;
      std::stringstream mss;
      mss << std::hex << mask;
      mss >> mask_val;
      int prefix = countSetBits(mask_val);
      rc.prefix = dest_ip + "/" + std::to_string(prefix);

      if (gateway != "00000000") {
        rc.nexthop = hexToIp(gateway);
      }

      rc.flags = std::stoul(flags, nullptr, 16);
      rc.rmx_mtu = std::stoi(mtu);

      routes.push_back(rc);
    }
  }

  // IPv6
  std::ifstream ipv6_route("/proc/net/ipv6_route");
  if (ipv6_route.is_open()) {
    std::string line;
    while (std::getline(ipv6_route, line)) {
      std::stringstream ss(line);
      std::string dest, dest_len, src, src_len, nexthop, metric, refcnt, use,
          flags, iface;
      ss >> dest >> dest_len >> src >> src_len >> nexthop >> metric >> refcnt >>
          use >> flags >> iface;

      RouteConfig rc;
      rc.iface = iface;

      // Parse IPv6 address from hex
      struct in6_addr addr;
      for (int i = 0; i < 16; ++i) {
        std::string byte = dest.substr(i * 2, 2);
        addr.s6_addr[i] = static_cast<uint8_t>(std::stoul(byte, nullptr, 16));
      }
      char buf[INET6_ADDRSTRLEN];
      inet_ntop(AF_INET6, &addr, buf, sizeof(buf));
      rc.prefix = std::string(buf) + "/" +
                  std::to_string(std::stoul(dest_len, nullptr, 16));

      if (nexthop != "00000000000000000000000000000000") {
        for (int i = 0; i < 16; ++i) {
          std::string byte = nexthop.substr(i * 2, 2);
          addr.s6_addr[i] = static_cast<uint8_t>(std::stoul(byte, nullptr, 16));
        }
        inet_ntop(AF_INET6, &addr, buf, sizeof(buf));
        rc.nexthop = buf;
      }

      rc.flags = std::stoul(flags, nullptr, 16);
      routes.push_back(rc);
    }
  }

  return routes;
}

std::vector<RouteConfig> SystemConfigurationManager::GetRoutes(
    const std::optional<VRFConfig> &vrf) const {
  return GetStaticRoutes(vrf);
}

void SystemConfigurationManager::AddRoute(const RouteConfig &route) const {
  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0)
    return;

  struct rtentry rt{};
  memset(&rt, 0, sizeof(rt));

  auto net = IPNetwork::fromString(route.prefix);
  if (!net) {
    close(sock);
    return;
  }

  struct sockaddr_in *dst = (struct sockaddr_in *)&rt.rt_dst;
  dst->sin_family = AF_INET;
  inet_pton(AF_INET, net->address()->toString().c_str(), &dst->sin_addr);

  struct sockaddr_in *mask = (struct sockaddr_in *)&rt.rt_genmask;
  mask->sin_family = AF_INET;
  auto m = IPAddress::maskFromCIDR(AddressFamily::IPv4, net->mask());
  if (m) {
    inet_pton(AF_INET, m->toString().c_str(), &mask->sin_addr);
  }

  if (route.nexthop) {
    struct sockaddr_in *gw = (struct sockaddr_in *)&rt.rt_gateway;
    gw->sin_family = AF_INET;
    inet_pton(AF_INET, route.nexthop->c_str(), &gw->sin_addr);
    rt.rt_flags |= RTF_GATEWAY;
  }

  rt.rt_flags |= RTF_UP;
  if (route.iface) {
    rt.rt_dev = const_cast<char *>(route.iface->c_str());
  }

  ioctl(sock, SIOCADDRT, &rt);
  close(sock);
}

void SystemConfigurationManager::DeleteRoute(const RouteConfig &route) const {
  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0)
    return;

  struct rtentry rt{};
  memset(&rt, 0, sizeof(rt));

  auto net = IPNetwork::fromString(route.prefix);
  if (!net) {
    close(sock);
    return;
  }

  struct sockaddr_in *dst = (struct sockaddr_in *)&rt.rt_dst;
  dst->sin_family = AF_INET;
  inet_pton(AF_INET, net->address()->toString().c_str(), &dst->sin_addr);

  struct sockaddr_in *mask = (struct sockaddr_in *)&rt.rt_genmask;
  mask->sin_family = AF_INET;
  auto m = IPAddress::maskFromCIDR(AddressFamily::IPv4, net->mask());
  if (m) {
    inet_pton(AF_INET, m->toString().c_str(), &mask->sin_addr);
  }

  ioctl(sock, SIOCDELRT, &rt);
  close(sock);
}
