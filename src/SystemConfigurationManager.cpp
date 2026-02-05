// Minimal static enumerator implementations for interface configs.

#include "SystemConfigurationManager.hpp"
#include "InterfaceConfig.hpp"
#include "LaggFlags.hpp"

#include <ifaddrs.h>
#include <net/route.h>
#include <netinet/in.h>
#include <net/if.h>
#include <optional>
#include <string>
#include <string_view>
#include <sys/sysctl.h>
#include <unordered_map>
#include <unordered_set>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/ethernet.h>
#include <net/if_lagg.h>
#include <sys/sockio.h>
#include <net/if_vlan_var.h>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <iostream>

// LAGG port flag labeling moved to include/LaggFlags.hpp

// Debug prints are always enabled (removed NETCLI_DEBUG env gating)

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

// Populate additional metadata for an interface by querying the system.
// Preferred implementation would use platform ioctls; fallback here uses
// `ifconfig <iface>` parsing to extract metric, groups, fib, tunnelfib,
// nd6 options, and for tunnels the endpoint addresses when available.
static void populateInterfaceMetadata(InterfaceConfig &ic) {
  // Use ioctls to retrieve metric, groups, fib, tunnelfib, nd6 options,
  // and tunnel endpoints (gif). No popen/ifconfig parsing.

  // Metric, fib, tunnelfib, and gif endpoints use an AF_INET socket.
  int inet_sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (inet_sock >= 0) {
    struct ifreq ifr;
    std::memset(&ifr, 0, sizeof(ifr));
    std::strncpy(ifr.ifr_name, ic.name.c_str(), IFNAMSIZ - 1);
    if (ioctl(inet_sock, SIOCGIFMETRIC, &ifr) == 0) {
      ic.metric = ifr.ifr_metric;
    }

    std::memset(&ifr, 0, sizeof(ifr));
    std::strncpy(ifr.ifr_name, ic.name.c_str(), IFNAMSIZ - 1);
    if (ioctl(inet_sock, SIOCGIFFIB, &ifr) == 0) {
      try {
        if (!ic.vrf)
          ic.vrf = std::make_unique<VRFConfig>();
        ic.vrf->table = ifr.ifr_fib;
        ic.vrf->name = std::string("fib") + std::to_string(ifr.ifr_fib);
      } catch (...) {}
    }

    std::memset(&ifr, 0, sizeof(ifr));
    std::strncpy(ifr.ifr_name, ic.name.c_str(), IFNAMSIZ - 1);
    if (ioctl(inet_sock, SIOCGTUNFIB, &ifr) == 0) {
      ic.tunnel_vrf = ifr.ifr_fib;
    }

    std::memset(&ifr, 0, sizeof(ifr));
    std::strncpy(ifr.ifr_name, ic.name.c_str(), IFNAMSIZ - 1);
    if (ioctl(inet_sock, SIOCGIFPSRCADDR, &ifr) == 0) {
      if (ifr.ifr_addr.sa_family == AF_INET) {
        struct sockaddr_in *sin = reinterpret_cast<struct sockaddr_in *>(&ifr.ifr_addr);
        char buf[INET_ADDRSTRLEN] = {0};
        if (inet_ntop(AF_INET, &sin->sin_addr, buf, sizeof(buf))) {
          try {
            if (!ic.tunnel) ic.tunnel = std::make_shared<TunnelConfig>(ic);
            ic.tunnel->source = std::make_unique<IPv4Address>(std::string(buf));
          } catch (...) {}
        }
      } else if (ifr.ifr_addr.sa_family == AF_INET6) {
        struct sockaddr_in6 *sin6 = reinterpret_cast<struct sockaddr_in6 *>(&ifr.ifr_addr);
        char buf[INET6_ADDRSTRLEN] = {0};
        if (inet_ntop(AF_INET6, &sin6->sin6_addr, buf, sizeof(buf))) {
          try {
            if (!ic.tunnel) ic.tunnel = std::make_shared<TunnelConfig>(ic);
            ic.tunnel->source = std::make_unique<IPv6Address>(std::string(buf));
          } catch (...) {}
        }
      }
    }

    std::memset(&ifr, 0, sizeof(ifr));
    std::strncpy(ifr.ifr_name, ic.name.c_str(), IFNAMSIZ - 1);
    if (ioctl(inet_sock, SIOCGIFPDSTADDR, &ifr) == 0) {
      if (ifr.ifr_addr.sa_family == AF_INET) {
        struct sockaddr_in *sin = reinterpret_cast<struct sockaddr_in *>(&ifr.ifr_addr);
        char buf[INET_ADDRSTRLEN] = {0};
        if (inet_ntop(AF_INET, &sin->sin_addr, buf, sizeof(buf))) {
          try {
            if (!ic.tunnel) ic.tunnel = std::make_shared<TunnelConfig>(ic);
            ic.tunnel->destination = std::make_unique<IPv4Address>(std::string(buf));
          } catch (...) {}
        }
      } else if (ifr.ifr_addr.sa_family == AF_INET6) {
        struct sockaddr_in6 *sin6 = reinterpret_cast<struct sockaddr_in6 *>(&ifr.ifr_addr);
        char buf[INET6_ADDRSTRLEN] = {0};
        if (inet_ntop(AF_INET6, &sin6->sin6_addr, buf, sizeof(buf))) {
          try {
            if (!ic.tunnel) ic.tunnel = std::make_shared<TunnelConfig>(ic);
            ic.tunnel->destination = std::make_unique<IPv6Address>(std::string(buf));
          } catch (...) {}
        }
      }
    }

    close(inet_sock);
  }

  // Use AF_LOCAL socket for group and status ioctls
  int local_sock = socket(AF_LOCAL, SOCK_DGRAM, 0);
  if (local_sock >= 0) {
    // Interface groups (SIOCGIFGROUP)
    struct ifgroupreq ifgr;
    std::memset(&ifgr, 0, sizeof(ifgr));
    std::strncpy(ifgr.ifgr_name, ic.name.c_str(), IFNAMSIZ - 1);
    if (ioctl(local_sock, SIOCGIFGROUP, &ifgr) == 0) {
      size_t len = ifgr.ifgr_len;
      if (len > 0) {
        size_t count = len / sizeof(struct ifg_req);
        struct ifg_req *groups = static_cast<struct ifg_req *>(std::calloc(count, sizeof(struct ifg_req)));
        if (groups) {
          ifgr.ifgr_groups = groups;
          if (ioctl(local_sock, SIOCGIFGROUP, &ifgr) == 0) {
            for (size_t i = 0; i < count; ++i) {
              std::string gname(groups[i].ifgrq_group);
              if (!gname.empty())
                ic.groups.emplace_back(gname);
            }
          }
          free(groups);
        }
      }
    }

    // Interface status (SIOCGIFSTATUS) to extract ND6 options and other text
    struct ifstat ifsstat;
    std::memset(&ifsstat, 0, sizeof(ifsstat));
    std::strncpy(ifsstat.ifs_name, ic.name.c_str(), IFNAMSIZ - 1);
    if (ioctl(local_sock, SIOCGIFSTATUS, &ifsstat) == 0) {
      std::string ascii(ifsstat.ascii);
      auto npos = ascii.find("nd6 options=");
      if (npos != std::string::npos) {
        auto lt = ascii.find('<', npos);
        auto gt = ascii.find('>', npos);
        if (lt != std::string::npos && gt != std::string::npos && gt > lt) {
          ic.nd6_options = ascii.substr(lt + 1, gt - lt - 1);
        }
      }
    }
    close(local_sock);
  }
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
  // Populate additional per-interface metadata from system and move entries
  for (auto &kv : map) {
    // Populate metric, nd6, tunnel endpoints, etc.
    populateInterfaceMetadata(kv.second);
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
    if (ic.type == InterfaceType::Bridge) {
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
    if (ic.type == InterfaceType::Lagg) {
      LaggConfig lac(ic);

      // Attempt to query kernel for LAGG protocol, ports and hash policy
      int sock = socket(AF_LOCAL, SOCK_DGRAM, 0);
      if (sock >= 0) {
        std::cerr << "GetLaggInterfaces: querying kernel for '" << ic.name << "'\n";
        // Use the same allocation/layout libifconfig uses to query lagg status
        struct _local_lagg_status {
          struct lagg_reqall ra;
          struct lagg_reqopts ro;
          struct lagg_reqflags rf;
          struct lagg_reqport rpbuf[LAGG_MAX_PORTS];
        } *ls = nullptr;

        ls = static_cast<decltype(ls)>(std::calloc(1, sizeof(*ls)));
        if (ls) {
          std::cerr << "GetLaggInterfaces: allocated status buffer\n";
          ls->ra.ra_port = ls->rpbuf;
          ls->ra.ra_size = sizeof(ls->rpbuf);
          std::strncpy(ls->ro.ro_ifname, ic.name.c_str(), IFNAMSIZ - 1);

          // Call SIOCGLAGGOPTS, SIOCGLAGGFLAGS, then SIOCGLAGG as libifconfig does.
          struct ifreq ifr;
          std::memset(&ifr, 0, sizeof(ifr));
          std::strncpy(ifr.ifr_name, ic.name.c_str(), IFNAMSIZ - 1);

          // SIOCGLAGGOPTS
          if (ioctl(sock, SIOCGLAGGOPTS, &ls->ro) != 0) {
            std::cerr << "GetLaggInterfaces: SIOCGLAGGOPTS failed: " << strerror(errno) << "\n";
          }

          // SIOCGLAGGFLAGS
          std::strncpy(ls->rf.rf_ifname, ic.name.c_str(), IFNAMSIZ - 1);
          if (ioctl(sock, SIOCGLAGGFLAGS, &ls->rf) != 0) {
            ls->rf.rf_flags = 0;
          }

          // SIOCGLAGG
          std::strncpy(ls->ra.ra_ifname, ic.name.c_str(), IFNAMSIZ - 1);
          if (ioctl(sock, SIOCGLAGG, &ls->ra) == 0) {
            std::cerr << "GetLaggInterfaces: SIOCGLAGG returned proto=" << ls->ra.ra_proto << " ports=" << ls->ra.ra_ports << "\n";
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
                lac.members.emplace_back(pname);
                lac.member_flag_bits.emplace_back(flags);
                std::cerr << "GetLaggInterfaces: found member '" << pname << "' flags=" << flags << "\n";
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
            if (!hp.empty()) {
              lac.hash_policy = hp;
              std::cerr << "GetLaggInterfaces: hash policy='" << hp << "'\n";
            }
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
              lac.member_flag_bits.emplace_back(rp.rp_flags);
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
    if (ic.type == InterfaceType::VLAN) {
      VLANConfig vconf(ic);

      // Query kernel for VLAN information using struct vlanreq via ioctl
      int vsock = socket(AF_INET, SOCK_DGRAM, 0);
      if (vsock >= 0) {
        std::cerr << "GetVLANInterfaces: querying kernel for '" << ic.name << "'\n";
        struct vlanreq vreq;
        std::memset(&vreq, 0, sizeof(vreq));
        struct ifreq ifr;
        std::memset(&ifr, 0, sizeof(ifr));
        std::strncpy(ifr.ifr_name, ic.name.c_str(), IFNAMSIZ - 1);
        ifr.ifr_data = reinterpret_cast<char *>(&vreq);

        if (ioctl(vsock, SIOCGETVLAN, &ifr) == 0) {
          vconf.id = static_cast<uint16_t>(vreq.vlr_tag & 0x0fff);
          int pcp = (vreq.vlr_tag >> 13) & 0x7;
          vconf.parent = std::string(vreq.vlr_parent);
          vconf.pcp = static_cast<PriorityCodePoint>(pcp);
          std::cerr << "GetVLANInterfaces: SIOCGETVLAN parent='" << vreq.vlr_parent << "' tag=" << vreq.vlr_tag << " proto=" << vreq.vlr_proto << "\n";

          // Map protocol if provided by ioctl
          if (vreq.vlr_proto) {
            uint16_t proto = static_cast<uint16_t>(vreq.vlr_proto);
            if (proto == 0x8100)
              vconf.proto = std::string("802.1q");
            else if (proto == 0x88a8)
              vconf.proto = std::string("802.1ad");
            else {
              char buf[8];
              std::snprintf(buf, sizeof(buf), "0x%04x", proto);
              vconf.proto = std::string(buf);
            }
          }
        } else {
          std::cerr << "GetVLANInterfaces: SIOCGETVLAN failed for '" << ic.name << "': " << strerror(errno) << "\n";
        }
        close(vsock);

        // Populate options (capabilities) from VLAN iface or parent
        auto query_caps = [&](const std::string &name) -> std::optional<std::string> {
          int csock = socket(AF_INET, SOCK_DGRAM, 0);
          if (csock < 0)
            return std::nullopt;
          struct ifreq cifr;
          std::memset(&cifr, 0, sizeof(cifr));
          std::strncpy(cifr.ifr_name, name.c_str(), IFNAMSIZ - 1);
            if (ioctl(csock, SIOCGIFCAP, &cifr) == 0) {
              unsigned int curcap = cifr.ifr_curcap;
              std::string opts;
              std::cerr << "GetVLANInterfaces: SIOCGIFCAP for '" << name << "' curcap=0x" << std::hex << curcap << std::dec << "\n";
            if (curcap & IFCAP_RXCSUM) {
              if (!opts.empty()) opts += ",";
              opts += "RXCSUM";
            }
            if (curcap & IFCAP_TXCSUM) {
              if (!opts.empty()) opts += ",";
              opts += "TXCSUM";
            }
            if (curcap & IFCAP_LINKSTATE) {
              if (!opts.empty()) opts += ",";
              opts += "LINKSTATE";
            }
            if (curcap & IFCAP_B_VLAN_HWTAGGING) {
              if (!opts.empty()) opts += ",";
              opts += "VLAN_HWTAG";
            }
            close(csock);
            if (!opts.empty()) {
              std::cerr << "GetVLANInterfaces: capabilities for '" << name << "' -> '" << opts << "'\n";
              return opts;
            }
          } else {
            close(csock);
          }
          return std::nullopt;
        };

        if (auto o = query_caps(ic.name); o) {
          vconf.options = *o;
          std::cerr << "GetVLANInterfaces: options for '" << ic.name << "' = '" << vconf.options.value() << "'\n";
        } else if (vconf.parent) {
          if (auto o = query_caps(*vconf.parent); o) {
            vconf.options = *o;
            std::cerr << "GetVLANInterfaces: options from parent '" << *vconf.parent << "' = '" << vconf.options.value() << "'\n";
          }
        }
      }

      out.emplace_back(std::move(vconf));
    }
  }
  return out;
}

std::vector<TunnelConfig> SystemConfigurationManager::GetTunnelInterfaces(
    const std::optional<VRFConfig> &vrf) const {
  auto bases = GetInterfaces(vrf);
  std::vector<TunnelConfig> out;
  for (const auto &ic : bases) {
    if (ic.type == InterfaceType::Tunnel || ic.type == InterfaceType::Gif || ic.type == InterfaceType::Tun) {
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
    if (ic.type == InterfaceType::Virtual || ic.type == InterfaceType::Tun) {
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
