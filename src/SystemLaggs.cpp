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

#include "LaggConfig.hpp"
#include "LaggFlags.hpp"
#include "Socket.hpp"
#include "SystemConfigurationManager.hpp"

#include <cstring>
#include <iostream>
#include <net/ethernet.h>
#include <net/if.h>
#include <net/if_lagg.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

bool SystemConfigurationManager::interfaceIsLagg(
    const std::string &ifname) const {
  try {
    Socket sock(AF_LOCAL, SOCK_DGRAM);

    struct _local_lagg_status {
      struct lagg_reqall ra;
      struct lagg_reqport rpbuf[LAGG_MAX_PORTS];
    } ls_buf{};
    ls_buf.ra.ra_port = ls_buf.rpbuf;
    ls_buf.ra.ra_size = sizeof(ls_buf.rpbuf);
    std::strncpy(ls_buf.ra.ra_ifname, ifname.c_str(), IFNAMSIZ - 1);

    return (ioctl(sock, SIOCGLAGG, &ls_buf.ra) == 0);
  } catch (...) {
    return false;
  }
}

std::vector<LaggConfig> SystemConfigurationManager::GetLaggInterfaces(
    const std::optional<VRFConfig> &vrf) const {
  auto bases = GetInterfaces(vrf);
  std::vector<LaggConfig> out;
  for (const auto &ic : bases) {
    LaggConfig lac(ic);

    try {
      Socket sock(AF_LOCAL, SOCK_DGRAM);

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

      ioctl(sock, SIOCGLAGGOPTS, &ls->ro);
      std::strncpy(ls->rf.rf_ifname, ic.name.c_str(), IFNAMSIZ - 1);
      if (ioctl(sock, SIOCGLAGGFLAGS, &ls->rf) != 0) {
        ls->rf.rf_flags = 0;
      }

      std::strncpy(ls->ra.ra_ifname, ic.name.c_str(), IFNAMSIZ - 1);
      if (ioctl(sock, SIOCGLAGG, &ls->ra) == 0) {
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
          case LAGG_PROTO_ROUNDROBIN:
            lac.protocol = LaggProtocol::ROUNDROBIN;
            break;
          case LAGG_PROTO_BROADCAST:
            lac.protocol = LaggProtocol::BROADCAST;
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
          if (!pname.empty()) {
            uint32_t flags = ls->rpbuf[i].rp_flags;
            lac.members.emplace_back(pname);
            lac.member_flag_bits.emplace_back(flags);
            // Convert known flag bits into human-readable label
            std::string lbl;
            if (flags & LAGG_PORT_MASTER) {
              if (!lbl.empty())
                lbl += ',';
              lbl += "MASTER";
            }
            if (flags & LAGG_PORT_STACK) {
              if (!lbl.empty())
                lbl += ',';
              lbl += "STACK";
            }
            if (flags & LAGG_PORT_ACTIVE) {
              if (!lbl.empty())
                lbl += ',';
              lbl += "ACTIVE";
            }
            if (flags & LAGG_PORT_COLLECTING) {
              if (!lbl.empty())
                lbl += ',';
              lbl += "COLLECTING";
            }
            if (flags & LAGG_PORT_DISTRIBUTING) {
              if (!lbl.empty())
                lbl += ',';
              lbl += "DISTRIBUTING";
            }
            lac.member_flags.emplace_back(lbl);
          }
        }

        uint32_t hf =
            ls->rf.rf_flags & (LAGG_F_HASHL2 | LAGG_F_HASHL3 | LAGG_F_HASHL4);
        if (hf) {
          lac.hash_policy = hf;
        }

        struct lagg_reqopts ro{};
        std::strncpy(ro.ro_ifname, ic.name.c_str(), IFNAMSIZ - 1);
        struct ifreq ifro;
        prepare_ifreq(ifro, ic.name);
        ifro.ifr_data = reinterpret_cast<char *>(&ro);
        if (ioctl(sock, SIOCGLAGGOPTS, &ifro) == 0) {
          lac.options = ro.ro_opts;
          lac.active_ports = static_cast<int>(ro.ro_active);
          lac.flapping = static_cast<int>(ro.ro_flapping);
          if (ro.ro_flowid_shift)
            lac.flowid_shift = ro.ro_flowid_shift;
          if (ro.ro_bkt)
            lac.rr_stride = ro.ro_bkt;
        }

        out.emplace_back(std::move(lac));
      }
    } catch (...) {
      // Socket creation failed, skip this interface
      continue;
    }
  }

  return out;
}

void SystemConfigurationManager::SaveLagg(const LaggConfig &lac) const {
  if (lac.name.empty())
    throw std::runtime_error("LaggConfig has no interface name set");

  if (!InterfaceConfig::exists(*this, lac.name))
    CreateLagg(lac.name);
  else
    SaveInterface(static_cast<const InterfaceConfig &>(lac));

  Socket sock(AF_INET, SOCK_DGRAM);

  int proto_value = 0;
  switch (lac.protocol) {
  case LaggProtocol::NONE:
    proto_value = LAGG_PROTO_NONE;
    break;
  case LaggProtocol::ROUNDROBIN:
    proto_value = LAGG_PROTO_ROUNDROBIN;
    break;
  case LaggProtocol::FAILOVER:
    proto_value = LAGG_PROTO_FAILOVER;
    break;
  case LaggProtocol::LOADBALANCE:
    proto_value = LAGG_PROTO_LOADBALANCE;
    break;
  case LaggProtocol::LACP:
    proto_value = LAGG_PROTO_LACP;
    break;
  case LaggProtocol::BROADCAST:
    proto_value = LAGG_PROTO_BROADCAST;
    break;
  }

  if (proto_value > 0) {
    struct lagg_reqall ra{};
    ra.ra_proto = proto_value;

    struct ifreq ifr;
    prepare_ifreq(ifr, lac.name);
    ifr.ifr_data = reinterpret_cast<char *>(&ra);

    if (ioctl(sock, SIOCSLAGG, &ifr) < 0) {
      throw std::runtime_error("Failed to set LAGG protocol: " +
                               std::string(strerror(errno)));
    }
  }

  for (const auto &member : lac.members) {
    struct lagg_reqport rp{};
    std::strncpy(rp.rp_portname, member.c_str(), IFNAMSIZ - 1);

    struct ifreq ifr;
    prepare_ifreq(ifr, lac.name);
    ifr.ifr_data = reinterpret_cast<char *>(&rp);

    if (ioctl(sock, SIOCSLAGGPORT, &ifr) < 0) {
      throw std::runtime_error("Failed to add port '" + member + "' to LAGG '" +
                               lac.name + "': " + std::string(strerror(errno)));
    }
  }

  if (lac.hash_policy) {
    std::cerr << "Note: Hash policy configuration for LAGG '" << lac.name
              << "' may require sysctl settings\n";
  }

  if (lac.lacp_rate) {
    std::cerr << "Note: LACP rate configuration for LAGG '" << lac.name
              << "' may require per-port settings\n";
  }
}

void SystemConfigurationManager::CreateLagg(const std::string &nm) const {
  cloneInterface(nm, SIOCIFCREATE);
}
