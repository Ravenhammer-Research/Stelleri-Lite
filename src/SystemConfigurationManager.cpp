// Minimal static enumerator implementations for interface configs.

#include "SystemConfigurationManager.hpp"
#include "InterfaceConfig.hpp"

#include <ifaddrs.h>
#include <net/route.h>
#include <netinet/in.h>
#include <optional>
#include <string>
#include <string_view>
#include <sys/sysctl.h>
#include <unordered_map>
#include <unordered_set>
#include <sys/ioctl.h>
#include <sys/socket.h>
#if defined(__has_include)
#if __has_include(<net/if_ether.h>)
#include <net/if_ether.h>
#elif __has_include(<net/ethernet.h>)
#include <net/ethernet.h>
#endif
#else
#include <net/if_ether.h>
#endif
#include <net/if_lagg.h>
#include <unistd.h>
#include <iostream>

static std::string laggPortFlagsLabel(uint32_t f) {
  std::string s;
  if (f & LAGG_PORT_MASTER) {
    if (!s.empty()) s += ",";
    s += "MASTER";
  }
  if (f & LAGG_PORT_STACK) {
    if (!s.empty()) s += ",";
    s += "STACK";
  }
  if (f & LAGG_PORT_ACTIVE) {
    if (!s.empty()) s += ",";
    s += "ACTIVE";
  }
  if (f & LAGG_PORT_COLLECTING) {
    if (!s.empty()) s += ",";
    s += "COLLECTING";
  }
  if (f & LAGG_PORT_DISTRIBUTING) {
    if (!s.empty()) s += ",";
    s += "DISTRIBUTING";
  }
  return s;
}

std::vector<RouteConfig> SystemConfigurationManager::GetStaticRoutes(
    const std::optional<VRFConfig> &vrf) const {
  std::vector<RouteConfig> routes;

  int mib[6] = {CTL_NET, PF_ROUTE, 0, AF_UNSPEC, NET_RT_DUMP, 0};
  size_t needed = 0;
  if (sysctl(mib, 6, nullptr, &needed, nullptr, 0) < 0 || needed == 0)
    return routes;

  std::vector<char> buf(needed);
  if (sysctl(mib, 6, buf.data(), &needed, nullptr, 0) < 0)
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
    struct sockaddr *rti_info[RTAX_MAX];
    std::memset(rti_info, 0, sizeof(rti_info));

    for (int i = 0;
         i < RTAX_MAX && reinterpret_cast<char *>(sa) < next + rtm->rtm_msglen;
         i++) {
      if ((rtm->rtm_addrs & (1 << i)) == 0)
        continue;
      rti_info[i] = sa;
#define ROUNDUP(a)                                                             \
  ((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))
      sa = reinterpret_cast<struct sockaddr *>(reinterpret_cast<char *>(sa) +
                                               ROUNDUP(sa->sa_len));
    }

    RouteConfig rc;

    if (rti_info[RTAX_DST]) {
      char buf_dst[INET6_ADDRSTRLEN] = {0};
      int prefixlen = 0;
      if (rti_info[RTAX_DST]->sa_family == AF_INET) {
        auto sin = reinterpret_cast<struct sockaddr_in *>(rti_info[RTAX_DST]);
        inet_ntop(AF_INET, &sin->sin_addr, buf_dst, sizeof(buf_dst));
        if (rti_info[RTAX_NETMASK]) {
          auto mask =
              reinterpret_cast<struct sockaddr_in *>(rti_info[RTAX_NETMASK]);
          uint32_t m = ntohl(mask->sin_addr.s_addr);
          prefixlen = __builtin_popcount(m);
        } else
          prefixlen = 32;
        rc.prefix = std::string(buf_dst) + "/" + std::to_string(prefixlen);
      } else if (rti_info[RTAX_DST]->sa_family == AF_INET6) {
        auto sin6 = reinterpret_cast<struct sockaddr_in6 *>(rti_info[RTAX_DST]);
        inet_ntop(AF_INET6, &sin6->sin6_addr, buf_dst, sizeof(buf_dst));
        if (rti_info[RTAX_NETMASK]) {
          auto mask6 =
              reinterpret_cast<struct sockaddr_in6 *>(rti_info[RTAX_NETMASK]);
          prefixlen = 0;
          for (int j = 0; j < 16; j++)
            prefixlen += __builtin_popcount(mask6->sin6_addr.s6_addr[j]);
        } else
          prefixlen = 128;
        rc.prefix = std::string(buf_dst) + "/" + std::to_string(prefixlen);
      }
    }

    if (rti_info[RTAX_GATEWAY]) {
      char buf_gw[INET6_ADDRSTRLEN] = {0};
      if (rti_info[RTAX_GATEWAY]->sa_family == AF_INET) {
        auto sin =
            reinterpret_cast<struct sockaddr_in *>(rti_info[RTAX_GATEWAY]);
        if (inet_ntop(AF_INET, &sin->sin_addr, buf_gw, sizeof(buf_gw)))
          rc.nexthop = buf_gw;
      } else if (rti_info[RTAX_GATEWAY]->sa_family == AF_INET6) {
        auto sin6 =
            reinterpret_cast<struct sockaddr_in6 *>(rti_info[RTAX_GATEWAY]);
        if (inet_ntop(AF_INET6, &sin6->sin6_addr, buf_gw, sizeof(buf_gw)))
          rc.nexthop = buf_gw;
      } else if (rti_info[RTAX_GATEWAY]->sa_family == AF_LINK) {
        auto sdl =
            reinterpret_cast<struct sockaddr_dl *>(rti_info[RTAX_GATEWAY]);
        if (sdl->sdl_nlen > 0)
          rc.iface = std::string(sdl->sdl_data, sdl->sdl_nlen);
      }
    }

    if (rtm->rtm_index > 0) {
      char ifname[IF_NAMESIZE];
      if (if_indextoname(rtm->rtm_index, ifname)) {
        if (!rc.iface)
          rc.iface = ifname;
      }
    }

    if (rtm->rtm_flags & RTF_BLACKHOLE)
      rc.blackhole = true;
    if (rtm->rtm_flags & RTF_REJECT)
      rc.reject = true;

    if (!rc.prefix.empty()) {
      // VRF filtering: kernel route messages here do not reliably include VRF
      // name so only include when no vrf filter requested; if vrf provided and
      // rc.vrf is unset, skip (conservative).
      if (vrf) {
        if (!rc.vrf) { /* cannot determine VRF for this route, skip */
        } else if (*rc.vrf != vrf->name) { /* skip */
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

std::vector<VRFConfig> SystemConfigurationManager::GetNetworkInstances(
    const std::optional<int> &table) const {
  std::vector<VRFConfig> out;
  // FreeBSD VRF/FIB enumeration is platform-specific and not exposed via a
  // simple sysctl on all systems. Provide a minimal implementation:
  // - if a specific table id is requested, return a VRFConfig with that table
  // - otherwise return empty (no enumeration implemented)
  if (table) {
    VRFConfig v;
    v.table = *table;
    // Name for the FIB is not universally defined; provide a helper name.
    v.name = std::string("fib") + std::to_string(*table);
    out.push_back(std::move(v));
  }
  return out;
}

std::vector<RouteConfig> SystemConfigurationManager::GetRoutes(
    const std::optional<VRFConfig> &vrf) const {
  // For now, alias to GetStaticRoutes
  return GetStaticRoutes(vrf);
}

static bool matches_vrf(const InterfaceConfig &ic,
                        const std::optional<VRFConfig> &vrf) {
  if (!vrf)
    return true;
  if (!ic.vrf)
    return false;
  if (ic.vrf->name != vrf->name)
    return false;
  if (vrf->table && ic.vrf->table)
    return *vrf->table == *ic.vrf->table;
  return true;
}

std::vector<InterfaceConfig> SystemConfigurationManager::GetInterfaces(
    const std::optional<VRFConfig> &vrf) const {
  std::vector<InterfaceConfig> out;
  struct ifaddrs *ifs = nullptr;
  if (getifaddrs(&ifs) != 0)
    return out;

  // Aggregate per-interface entries: some systems provide multiple ifaddrs
  // entries per logical interface (link, IPv4, IPv6). Build a map and merge
  // address entries so primary and aliases are captured.
  std::unordered_map<std::string, InterfaceConfig> map;
  for (struct ifaddrs *ifa = ifs; ifa; ifa = ifa->ifa_next) {
    if (!ifa->ifa_name)
      continue;
    std::string name = ifa->ifa_name;
    auto it = map.find(name);
    if (it == map.end()) {
      // First time seeing this interface: construct from this ifa
      InterfaceConfig ic(ifa);
      map.emplace(name, std::move(ic));
    } else {
      // Merge address information from subsequent ifaddrs entries
      InterfaceConfig &existing = it->second;
      if (ifa->ifa_addr) {
        if (ifa->ifa_addr->sa_family == AF_INET ||
            ifa->ifa_addr->sa_family == AF_INET6) {
          InterfaceConfig tmp(ifa);
          if (tmp.address) {
            if (!existing.address) {
              existing.address = std::move(tmp.address);
            } else {
              existing.aliases.emplace_back(std::move(tmp.address));
            }
          }
        }
      }
    }
  }

  // Move entries into output vector if they match VRF filter
  for (auto &kv : map) {
    if (matches_vrf(kv.second, vrf))
      out.emplace_back(std::move(kv.second));
  }

  freeifaddrs(ifs);
  return out;
}

std::vector<BridgeInterfaceConfig>
SystemConfigurationManager::GetBridgeInterfaces(
    const std::optional<VRFConfig> &vrf) const {
  auto bases = GetInterfaces(vrf);
  std::vector<BridgeInterfaceConfig> out;
  for (const auto &ic : bases) {
    if (ic.type == InterfaceType::Bridge || ic.name.rfind("bridge", 0) == 0) {
      BridgeInterfaceConfig bic(ic);
      bic.loadMembers();
      out.emplace_back(std::move(bic));
    }
  }
  return out;
}

std::vector<LaggConfig> SystemConfigurationManager::GetLaggInterfaces(
    const std::optional<VRFConfig> &vrf) const {
  auto bases = GetInterfaces(vrf);
  std::vector<LaggConfig> out;
  for (const auto &ic : bases) {
    if (ic.name.rfind("lagg", 0) == 0) {
      LaggConfig lac(ic);

      // Attempt to query kernel for LAGG protocol, ports and hash policy
      int sock = socket(AF_LOCAL, SOCK_DGRAM, 0);
      if (sock >= 0) {
        // Debug: querying kernel for interface
        // std::cerr << "GetLaggInterfaces: querying kernel for '" << ic.name << "'\n";
        // Use the same allocation/layout libifconfig uses to query lagg status
        struct _local_lagg_status {
          struct lagg_reqall ra;
          struct lagg_reqopts ro;
          struct lagg_reqflags rf;
          struct lagg_reqport rpbuf[LAGG_MAX_PORTS];
        } *ls = nullptr;

        ls = static_cast<decltype(ls)>(std::calloc(1, sizeof(*ls)));
        if (ls) {
          ls->ra.ra_port = ls->rpbuf;
          ls->ra.ra_size = sizeof(ls->rpbuf);
          std::strncpy(ls->ro.ro_ifname, ic.name.c_str(), IFNAMSIZ - 1);

          // Call SIOCGLAGGOPTS, SIOCGLAGGFLAGS, then SIOCGLAGG as libifconfig does.
          struct ifreq ifr;
          std::memset(&ifr, 0, sizeof(ifr));
          std::strncpy(ifr.ifr_name, ic.name.c_str(), IFNAMSIZ - 1);

          // SIOCGLAGGOPTS
          if (ioctl(sock, SIOCGLAGGOPTS, &ls->ro) != 0) {
            // std::cerr << "GetLaggInterfaces: SIOCGLAGGOPTS failed: " << strerror(errno) << "\n";
          }

          // SIOCGLAGGFLAGS
          std::strncpy(ls->rf.rf_ifname, ic.name.c_str(), IFNAMSIZ - 1);
          if (ioctl(sock, SIOCGLAGGFLAGS, &ls->rf) != 0) {
            ls->rf.rf_flags = 0;
          }

          // SIOCGLAGG
          std::strncpy(ls->ra.ra_ifname, ic.name.c_str(), IFNAMSIZ - 1);
          if (ioctl(sock, SIOCGLAGG, &ls->ra) == 0) {
            // std::cerr << "GetLaggInterfaces: SIOCGLAGG returned proto=" << ls->ra.ra_proto << " ports=" << ls->ra.ra_ports << "\n";
            if (ls->ra.ra_proto) {
              switch (ls->ra.ra_proto) {
              case LAGG_PROTO_FAILOVER:
                lac.protocol = LaggProtocol::FAILOVER;
                break;
              case LAGG_PROTO_LOADBALANCE:
                lac.protocol = LaggProtocol::LOADBALANCE;
                break;
              case LAGG_PROTO_LACP:
                lac.protocol = LaggProtocol::LACP;
                break;
              default:
                lac.protocol = LaggProtocol::NONE;
                break;
              }
            }

            int nports = ls->ra.ra_ports;
            if (nports > LAGG_MAX_PORTS)
              nports = LAGG_MAX_PORTS;
            for (int i = 0; i < nports; ++i) {
              std::string pname = ls->rpbuf[i].rp_portname;
              // std::cerr << "GetLaggInterfaces: lp port[" << i << "] name='" << pname << "' flags=" << ls->rpbuf[i].rp_flags << "\n";
              if (!pname.empty()) {
                uint32_t flags = ls->rpbuf[i].rp_flags;
                std::string lbl = laggPortFlagsLabel(flags);
                lac.members.emplace_back(pname);
                lac.member_flags.emplace_back(lbl.empty() ? std::string("-") : lbl);
              }
            }
          } else {
            // std::cerr << "GetLaggInterfaces: SIOCGLAGG failed: " << strerror(errno) << "\n";
          }

          // hash flags from ls->rf
          {
            std::string hp;
            if (ls->rf.rf_flags & LAGG_F_HASHL2)
              hp += (hp.empty() ? "l2" : ",l2");
            if (ls->rf.rf_flags & LAGG_F_HASHL3)
              hp += (hp.empty() ? "l3" : ",l3");
            if (ls->rf.rf_flags & LAGG_F_HASHL4)
              hp += (hp.empty() ? "l4" : ",l4");
            if (!hp.empty())
              lac.hash_policy = hp;
          }

          free(ls);
        }

        // If no members found yet, try per-port query via SIOCGLAGGPORT
        if (lac.members.empty()) {
          // std::cerr << "GetLaggInterfaces: no members from SIOCGLAGG, scanning interfaces with SIOCGLAGGPORT\n";
          for (const auto &candidate : bases) {
            struct lagg_reqport rp;
            std::memset(&rp, 0, sizeof(rp));
            std::strncpy(rp.rp_ifname, ic.name.c_str(), IFNAMSIZ - 1);
            std::strncpy(rp.rp_portname, candidate.name.c_str(), IFNAMSIZ - 1);

            if (ioctl(sock, SIOCGLAGGPORT, &rp) == 0) {
              // std::cerr << "GetLaggInterfaces: SIOCGLAGGPORT found port '" << rp.rp_portname
              //           << "' flags=" << rp.rp_flags << " for lagg '" << ic.name << "'\n";
              lac.members.emplace_back(std::string(rp.rp_portname));
              std::string lbl = laggPortFlagsLabel(rp.rp_flags);
              lac.member_flags.emplace_back(lbl.empty() ? std::string("-") : lbl);
            }
          }
        }

        // Also query options (counts) for additional info
        struct lagg_reqopts ro;
        std::memset(&ro, 0, sizeof(ro));
        std::strncpy(ro.ro_ifname, ic.name.c_str(), IFNAMSIZ - 1);
        struct ifreq ifro;
        std::memset(&ifro, 0, sizeof(ifro));
        std::strncpy(ifro.ifr_name, ic.name.c_str(), IFNAMSIZ - 1);
        ifro.ifr_data = reinterpret_cast<char *>(&ro);
        if (ioctl(sock, SIOCGLAGGOPTS, &ifro) == 0) {
          // std::cerr << "GetLaggInterfaces: SIOCGLAGGOPTS ro_count=" << ro.ro_count
          //           << " ro_active=" << ro.ro_active << " ro_opts=" << ro.ro_opts << "\n";
        } else {
          // std::cerr << "GetLaggInterfaces: SIOCGLAGGOPTS ioctl failed for '" << ic.name
          //           << "': " << strerror(errno) << "\n";
        }

        // Query hash flags
        struct lagg_reqflags rf;
        std::memset(&rf, 0, sizeof(rf));
        std::strncpy(rf.rf_ifname, ic.name.c_str(), IFNAMSIZ - 1);
        struct ifreq ifrf;
        std::memset(&ifrf, 0, sizeof(ifrf));
        std::strncpy(ifrf.ifr_name, ic.name.c_str(), IFNAMSIZ - 1);
        ifrf.ifr_data = reinterpret_cast<char *>(&rf);
        if (ioctl(sock, SIOCGLAGGFLAGS, &ifrf) == 0) {
          // std::cerr << "GetLaggInterfaces: SIOCGLAGGFLAGS returned flags=" << rf.rf_flags << "\n";
          std::string hp;
          if (rf.rf_flags & LAGG_F_HASHL2)
            hp += (hp.empty() ? "l2" : ",l2");
          if (rf.rf_flags & LAGG_F_HASHL3)
            hp += (hp.empty() ? "l3" : ",l3");
          if (rf.rf_flags & LAGG_F_HASHL4)
            hp += (hp.empty() ? "l4" : ",l4");
          if (!hp.empty())
            lac.hash_policy = hp;
        } else {
          // std::cerr << "GetLaggInterfaces: SIOCGLAGGFLAGS ioctl failed for '" << ic.name
          //           << "': " << strerror(errno) << "\n";
        }

        close(sock);
      }

      out.emplace_back(std::move(lac));
    }
  }
  return out;
}

std::vector<VLANConfig> SystemConfigurationManager::GetVLANInterfaces(
    const std::optional<VRFConfig> &vrf) const {
  auto bases = GetInterfaces(vrf);
  std::vector<VLANConfig> out;
  for (const auto &ic : bases) {
    if (ic.type == InterfaceType::VLAN || ic.name.rfind("vlan", 0) == 0) {
      out.emplace_back(ic);
    }
  }
  return out;
}

std::vector<TunnelConfig> SystemConfigurationManager::GetTunnelInterfaces(
    const std::optional<VRFConfig> &vrf) const {
  auto bases = GetInterfaces(vrf);
  std::vector<TunnelConfig> out;
  for (const auto &ic : bases) {
    if (ic.type == InterfaceType::Tunnel || ic.name.rfind("gif", 0) == 0) {
      out.emplace_back(ic);
    }
  }
  return out;
}

std::vector<VirtualInterfaceConfig>
SystemConfigurationManager::GetVirtualInterfaces(
    const std::optional<VRFConfig> &vrf) const {
  auto bases = GetInterfaces(vrf);
  std::vector<VirtualInterfaceConfig> out;
  for (const auto &ic : bases) {
    if (ic.type == InterfaceType::Virtual || ic.name.rfind("epair", 0) == 0 ||
        ic.name.rfind("tap", 0) == 0 || ic.name.rfind("tun", 0) == 0) {
      out.emplace_back(ic);
    }
  }
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
