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
#include "SystemConfigurationManager.hpp"

#include <cstring>
#include <net/ethernet.h>
#include <net/if.h>
#include <net/if_lagg.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

bool SystemConfigurationManager::interfaceIsLagg(
    const std::string &ifname) const {
  int sock = socket(AF_LOCAL, SOCK_DGRAM, 0);
  if (sock < 0)
    return false;

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

std::vector<LaggConfig> SystemConfigurationManager::GetLaggInterfaces(
    const std::optional<VRFConfig> &vrf) const {
  auto bases = GetInterfaces(vrf);
  std::vector<LaggConfig> out;
  for (const auto &ic : bases) {
    LaggConfig lac(ic);

    int sock = socket(AF_LOCAL, SOCK_DGRAM, 0);
    if (sock < 0) {
      continue;
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
        }
      }

      uint32_t hf =
          ls->rf.rf_flags & (LAGG_F_HASHL2 | LAGG_F_HASHL3 | LAGG_F_HASHL4);
      if (hf) {
        lac.hash_policy = hf;
      }

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
