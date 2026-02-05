// SystemConfigurationManager implementation.
// Provides platform-level enumeration helpers for interfaces, routes,
// VLANs, LAGG interfaces and related metadata.

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
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/ethernet.h>
#include <net/if_lagg.h>
#include <net/if_bridgevar.h>
#include <sys/sockio.h>
#include <net/if_vlan_var.h>
#include <cstring>
#include <unistd.h>

// LAGG port flag labeling moved to include/LaggFlags.hpp

// Prepare an `ifreq` with the provided interface name.
static void prepare_ifreq(struct ifreq &ifr, const std::string &name) {
  std::memset(&ifr, 0, sizeof(ifr));
  std::strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ - 1);
}

// Enum selecting which integer field to read from an `ifreq`.
enum class IfreqIntField { Metric, Fib, Mtu };

// Query an integer-valued `ifreq` attribute (metric, fib, mtu).
static std::optional<int> query_ifreq_int(const std::string &ifname, unsigned long req, IfreqIntField which) {
  int s = socket(AF_INET, SOCK_DGRAM, 0);
  if (s < 0) return std::nullopt;
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

// Query an `ifreq` that returns a socket address and convert it to
// a text representation. Returns (address, family) on success.
static std::optional<std::pair<std::string,int>> query_ifreq_sockaddr(const std::string &ifname, unsigned long req) {
  int s = socket(AF_INET, SOCK_DGRAM, 0);
  if (s < 0) return std::nullopt;
  struct ifreq ifr;
  prepare_ifreq(ifr, ifname);
  if (ioctl(s, req, &ifr) == 0) {
    if (ifr.ifr_addr.sa_family == AF_INET) {
      struct sockaddr_in *sin = reinterpret_cast<struct sockaddr_in *>(&ifr.ifr_addr);
      char buf[INET_ADDRSTRLEN] = {0};
      if (inet_ntop(AF_INET, &sin->sin_addr, buf, sizeof(buf))) {
        close(s);
        return std::make_pair(std::string(buf), AF_INET);
      }
    } else if (ifr.ifr_addr.sa_family == AF_INET6) {
      struct sockaddr_in6 *sin6 = reinterpret_cast<struct sockaddr_in6 *>(&ifr.ifr_addr);
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

// Obtain interface group membership via SIOCGIFGROUP. Returns group
// names associated with `ifname`.
static std::vector<std::string> query_interface_groups(const std::string &ifname) {
  std::vector<std::string> out;
  int s = socket(AF_LOCAL, SOCK_DGRAM, 0);
  if (s < 0) return out;
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
          if (!gname.empty()) out.emplace_back(gname);
        }
      }
    }
  }
  close(s);
  return out;
}

// Detect whether an interface is a kernel LAGG by attempting SIOCGLAGG.
static bool interfaceIsLagg(const std::string &ifname) {
  int sock = socket(AF_LOCAL, SOCK_DGRAM, 0);
  if (sock < 0) return false;

  struct _local_lagg_status {
    struct lagg_reqall ra;
    struct lagg_reqport rpbuf[LAGG_MAX_PORTS];
  } ls_buf{};
  std::memset(&ls_buf, 0, sizeof(ls_buf));
  ls_buf.ra.ra_port = ls_buf.rpbuf;
  ls_buf.ra.ra_size = sizeof(ls_buf.rpbuf);
  std::strncpy(ls_buf.ra.ra_ifname, ifname.c_str(), IFNAMSIZ - 1);

  bool res = false;
  if (ioctl(sock, SIOCGLAGG, &ls_buf.ra) == 0) {
    res = true;
  }
  close(sock);
  return res;
}

// Detect whether an interface is a kernel bridge by attempting SIOCGDRVSPEC
// with BRDGGIFS (get bridge member list). If the ioctl succeeds, treat the
// interface as a bridge.
static bool interfaceIsBridge(const std::string &ifname) {
  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0) return false;

  // Try with a small buffer; success means it's a bridge.
  const size_t entries = 8;
  std::vector<struct ifbreq> buf(entries);
  std::memset(buf.data(), 0, buf.size() * sizeof(struct ifbreq));

  struct ifbifconf ifbic;
  std::memset(&ifbic, 0, sizeof(ifbic));
  ifbic.ifbic_len = static_cast<uint32_t>(buf.size() * sizeof(struct ifbreq));
  ifbic.ifbic_buf = reinterpret_cast<caddr_t>(buf.data());

  struct ifdrv ifd;
  std::memset(&ifd, 0, sizeof(ifd));
  std::strncpy(ifd.ifd_name, ifname.c_str(), IFNAMSIZ - 1);
  ifd.ifd_cmd = BRDGGIFS;
  ifd.ifd_len = static_cast<int>(sizeof(ifbic));
  ifd.ifd_data = &ifbic;

  bool res = false;
  if (ioctl(sock, SIOCGDRVSPEC, &ifd) == 0) {
    res = true;
  }

  close(sock);
  return res;
}

// Attempt to extract ND6 (IPv6 neighbor discovery) option flags for
// an interface. This helper currently does not perform the ioctl and
// returns `std::nullopt` to avoid platform ABI assumptions.
static std::optional<std::string> query_ifstatus_nd6(const std::string &ifname) {
  (void)ifname;
  return std::nullopt;
}

// Diagnostic logging minimized in helpers; callers may emit logs.

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
      // VRF filtering: kernel route records may not include VRF names.
      // If a VRF filter is supplied and the route record lacks VRF info,
      // skip the route to be conservative.
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

// Return true if interface `ic` belongs to the optional `vrf` filter.
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

// Fill additional per-interface metadata (metric, VRF/FIB, tunnel
// endpoints, groups). This uses ioctl/getifaddrs-based queries and
// avoids calling external tools.
static void populateInterfaceMetadata(InterfaceConfig &ic) {

  // Metric, fib, tunnelfib, and gif endpoints use ioctls via helpers.
  if (auto m = query_ifreq_int(ic.name, SIOCGIFMETRIC, IfreqIntField::Metric); m) {
    ic.metric = *m;
  }
  if (auto f = query_ifreq_int(ic.name, SIOCGIFFIB, IfreqIntField::Fib); f) {
    if (!ic.vrf) ic.vrf = std::make_unique<VRFConfig>();
    ic.vrf->table = *f;
    ic.vrf->name = std::string("fib") + std::to_string(*f);
  }
  if (auto tf = query_ifreq_int(ic.name, SIOCGTUNFIB, IfreqIntField::Fib); tf) {
    ic.tunnel_vrf = *tf;
  }

  if (auto src = query_ifreq_sockaddr(ic.name, SIOCGIFPSRCADDR); src) {
    if (src->second == AF_INET) {
      if (!ic.tunnel) ic.tunnel = std::make_shared<TunnelConfig>(ic);
      ic.tunnel->source = std::make_unique<IPv4Address>(src->first);
    } else if (src->second == AF_INET6) {
      if (!ic.tunnel) ic.tunnel = std::make_shared<TunnelConfig>(ic);
      ic.tunnel->source = std::make_unique<IPv6Address>(src->first);
    }
  }

  if (auto dst = query_ifreq_sockaddr(ic.name, SIOCGIFPDSTADDR); dst) {
    if (dst->second == AF_INET) {
      if (!ic.tunnel) ic.tunnel = std::make_shared<TunnelConfig>(ic);
      ic.tunnel->destination = std::make_unique<IPv4Address>(dst->first);
    } else if (dst->second == AF_INET6) {
      if (!ic.tunnel) ic.tunnel = std::make_shared<TunnelConfig>(ic);
      ic.tunnel->destination = std::make_unique<IPv6Address>(dst->first);
    }
  }

  // Retrieve group membership and ND6 options using helper functions.
  auto groups = query_interface_groups(ic.name);
  for (const auto &g : groups) ic.groups.emplace_back(g);
  if (auto nd6 = query_ifstatus_nd6(ic.name); nd6) ic.nd6_options = *nd6;
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
    // If the kernel reports this interface as a LAGG or a bridge, mark its
    // type so formatters display the appropriate specialized type.
    if (interfaceIsLagg(kv.second.name)) {
      kv.second.type = InterfaceType::Lagg;
    } else if (interfaceIsBridge(kv.second.name)) {
      kv.second.type = InterfaceType::Bridge;
    }
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
    // Attempt to query the kernel for LAGG state for this interface. Some
    // systems may not expose explicit link-layer types for LAGG in the
    // `ifaddrs` records, so detect LAGG by trying the SIOCGLAGG ioctl.
    LaggConfig lac(ic);

    int sock = socket(AF_LOCAL, SOCK_DGRAM, 0);
    if (sock < 0) {
      continue; // cannot probe this interface
    }

    struct _local_lagg_status {
      struct lagg_reqall ra;
      struct lagg_reqopts ro;
      struct lagg_reqflags rf;
      struct lagg_reqport rpbuf[LAGG_MAX_PORTS];
    };
    _local_lagg_status ls_buf{};
    _local_lagg_status *ls = &ls_buf;
    ls->ra.ra_port = ls->rpbuf;
    ls->ra.ra_size = sizeof(ls->rpbuf);
    std::strncpy(ls->ro.ro_ifname, ic.name.c_str(), IFNAMSIZ - 1);

    // Try to read options/flags (best-effort)
    ioctl(sock, SIOCGLAGGOPTS, &ls->ro);
    std::strncpy(ls->rf.rf_ifname, ic.name.c_str(), IFNAMSIZ - 1);
    if (ioctl(sock, SIOCGLAGGFLAGS, &ls->rf) != 0) {
      ls->rf.rf_flags = 0;
    }

    std::strncpy(ls->ra.ra_ifname, ic.name.c_str(), IFNAMSIZ - 1);
    if (ioctl(sock, SIOCGLAGG, &ls->ra) == 0) {
      // We successfully queried LAGG state: treat this interface as LAGG.
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
      if (nports > LAGG_MAX_PORTS) nports = LAGG_MAX_PORTS;
      for (int i = 0; i < nports; ++i) {
        std::string pname = ls->rpbuf[i].rp_portname;
        if (!pname.empty()) {
          uint32_t flags = ls->rpbuf[i].rp_flags;
          lac.members.emplace_back(pname);
          lac.member_flag_bits.emplace_back(flags);
        }
      }

      uint32_t hf = ls->rf.rf_flags & (LAGG_F_HASHL2 | LAGG_F_HASHL3 | LAGG_F_HASHL4);
      if (hf) {
        lac.hash_policy = hf;
      }

      // Try SIOCGLAGGOPTS via ifreq (best-effort)
      struct lagg_reqopts ro;
      std::memset(&ro, 0, sizeof(ro));
      std::strncpy(ro.ro_ifname, ic.name.c_str(), IFNAMSIZ - 1);
      struct ifreq ifro;
      std::memset(&ifro, 0, sizeof(ifro));
      std::strncpy(ifro.ifr_name, ic.name.c_str(), IFNAMSIZ - 1);
      ifro.ifr_data = reinterpret_cast<char *>(&ro);
      ioctl(sock, SIOCGLAGGOPTS, &ifro);

      out.emplace_back(std::move(lac));
    }

    close(sock);
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
        (void)0; // kernel query
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
          (void)0;

          // Map protocol if provided by ioctl into enum
          if (vreq.vlr_proto) {
            uint16_t proto = static_cast<uint16_t>(vreq.vlr_proto);
            if (proto == static_cast<uint16_t>(VLANProto::DOT1Q))
              vconf.proto = VLANProto::DOT1Q;
            else if (proto == static_cast<uint16_t>(VLANProto::DOT1AD))
              vconf.proto = VLANProto::DOT1AD;
            else
              vconf.proto = VLANProto::OTHER;
          }
        } else {
          (void)0;
        }
        close(vsock);

        // Populate options (capabilities) from VLAN iface or parent as raw bits
        auto query_caps = [&](const std::string &name) -> std::optional<uint32_t> {
          int csock = socket(AF_INET, SOCK_DGRAM, 0);
          if (csock < 0)
            return std::nullopt;
          struct ifreq cifr;
          std::memset(&cifr, 0, sizeof(cifr));
          std::strncpy(cifr.ifr_name, name.c_str(), IFNAMSIZ - 1);
          if (ioctl(csock, SIOCGIFCAP, &cifr) == 0) {
            unsigned int curcap = cifr.ifr_curcap;
            (void)curcap;
            close(csock);
            return static_cast<uint32_t>(curcap);
          } else {
            close(csock);
            return std::nullopt;
          }
        };

        if (auto o = query_caps(ic.name); o) {
          vconf.options_bits = *o;
            (void)0;
        } else if (vconf.parent) {
          if (auto o = query_caps(*vconf.parent); o) {
            vconf.options_bits = *o;
            (void)0;
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
